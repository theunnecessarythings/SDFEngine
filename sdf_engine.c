#include <Python.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include "cglm/cglm.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define p 1.f
GLuint vertexArrayID, programID, vertexBuffer;

GLuint channel_0, channel_1, channel_2, channel_3;

// Assumes the file exists and will seg. fault otherwise.
const GLchar *load_shader_source(const char *filename) {
  FILE *file = fopen(filename, "r");             // open 
  fseek(file, 0L, SEEK_END);                     // find the end
  size_t size = ftell(file);                     // get the size in bytes
  GLchar *shaderSource = calloc(1, size);        // allocate enough bytes
  rewind(file);                                  // go back to file beginning
  int ret = fread(shaderSource, size, sizeof(char), file); // read each char into ourblock
  fclose(file);                                  // close the stream
  return shaderSource;
}

void createTextures(const char* image_path) {
   int width, height, nrChannels;
   unsigned char *data = stbi_load(image_path, &width, &height, &nrChannels, 0);
   glGenTextures(1, &channel_0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, channel_0);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glGenerateMipmap(GL_TEXTURE_2D);
}


GLuint loadShaders(const char * vertexSourcePointer,const char * fragmentSourcePointer){

	// Create the shaders
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	
	GLint result = GL_FALSE;
	int infoLogLength;

   GLchar const* files_v[] = {vertexSourcePointer};
	// Compile Vertex Shader
	printf("Compiling Vertex Shader\n");
	glShaderSource(vertexShaderID, 1, files_v , NULL);
	glCompileShader(vertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if ( infoLogLength > 0 ){
		printf("Vertex Shader Compilation Failed\n");
      char errorLog[512];
	   glGetShaderInfoLog(vertexShaderID, infoLogLength, &infoLogLength, &errorLog[0]);
      printf("%s\n", errorLog);
      printf("%d\n", result);
	}


   GLchar const* files_f[] = {fragmentSourcePointer};
	// Compile Fragment Shader
	printf("Compiling Fragment Shader\n");
	glShaderSource(fragmentShaderID, 1, files_f , NULL);
	glCompileShader(fragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if ( infoLogLength > 0 ){
		printf("Fragment Shader Compilation Failed\n");
      char errorLog[512];
	   glGetShaderInfoLog(fragmentShaderID, infoLogLength, &infoLogLength, &errorLog[0]);
      printf("%s\n", errorLog);
      printf("%d\n", result);
	}



	// Link the program
	printf("Linking program\n");
	GLuint programID = glCreateProgram();
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	// Check the program
	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if ( infoLogLength > 0 ){
		printf("Shader Linking Failed\n");
      char errorLog[512];
	   glGetProgramInfoLog(programID, infoLogLength, &infoLogLength, &errorLog[0]);
      printf("%s\n", errorLog);
      printf("%d\n", result);
	}

	
	glDetachShader(programID, vertexShaderID);
	glDetachShader(programID, fragmentShaderID);
	
	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	return programID;
}

static PyObject* glCompileProgram(PyObject* self, PyObject* args) {
   GLchar *vertex_shader, *fragment_shader;
   if (!PyArg_ParseTuple(args, "ss", &vertex_shader, &fragment_shader))
      return NULL;
   // Create and compile our GLSL program from the shaders
	//programID = loadShaders( "/home/night-queen/Downloads/of_v0.10.1_linux64gcc6_release/apps/myApps/mySketch/bin/data/SimpleVertexShader.vertexshader", "/home/night-queen/Downloads/of_v0.10.1_linux64gcc6_release/apps/myApps/mySketch/bin/data/SimpleFragmentShader.fragmentshader" );
	programID = loadShaders(vertex_shader, fragment_shader);
   return Py_BuildValue("i", programID);
}

static PyObject* glProgram(PyObject* self, PyObject* args) {
   GLuint programID;
   if (!PyArg_ParseTuple(args, "i", &programID))
      return NULL;
   glUseProgram(0);
   glUseProgram(programID);
   return Py_None;
}

static PyObject* glRenderInit(PyObject* self, PyObject* args) {
   GLuint width, height;
   if (!PyArg_ParseTuple(args, "ii", &width, &height))
      return NULL;
   GLuint fb, color, depth;
	if (!glfwInit()) {

		printf("Initialising GLFW failed\n");

	}
   
	GLFWwindow* window;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
   window = glfwCreateWindow( width, height, "Hidden Window", NULL, NULL);
   glfwMakeContextCurrent(window);
   glewExperimental = 1;
	if (glewInit() != GLEW_OK) {
		printf("GLEW initialisation failed\n");
	}
   

   glGenFramebuffers(1, &fb);
   glGenTextures(1, &color);
   glGenRenderbuffers(1, &depth);

   glBindFramebuffer(GL_FRAMEBUFFER, fb);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, color);
   glTexImage2D(   GL_TEXTURE_2D, 
         0, 
         GL_SRGB_ALPHA, 
         width, height,
         0, 
         GL_RGBA, 
         GL_FLOAT, 
         NULL);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, depth);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
   glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
   GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT};
   glDrawBuffers(2, DrawBuffers); 

   GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
   status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER); 
   switch(status) {
      case GL_FRAMEBUFFER_COMPLETE:
         break;
      case GL_FRAMEBUFFER_UNSUPPORTED:
         break;
      default:
         fputs("Framebuffer Error\n", stderr);
         printf("%d\n", glGetError());
   }
   return Py_None;
}

static PyObject* glRenderResult(PyObject* self, PyObject* args) {
   // switch back to window-system-provided framebuffer
   GLuint width, height;
   if (!PyArg_ParseTuple(args, "ii", &width, &height))
      return NULL;
   GLfloat *pixels, *depth;
   pixels = (GLfloat*) malloc(width * height * 4 * sizeof(GLfloat));
   depth = (GLfloat*) malloc(width * height * sizeof(GLfloat));
   //glReadBuffer(GL_NONE);
   glReadPixels( 0,0,  width, height, GL_RGBA, GL_FLOAT, pixels);
   glReadPixels( 0,0,  width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   int N = width * height;
   PyObject* pixel_list = PyList_New(N);
   for (int i = 0; i < N; ++i)
   {
      PyObject* pixel_val = Py_BuildValue("ffff", pixels[i*4], pixels[i*4+1], pixels[i*4+2], pixels[i*4+3]);
      PyList_SetItem(pixel_list, i, pixel_val);
   }
   PyObject* depth_list = PyList_New(N);
   for (int i = 0; i < N; ++i)
   {
      PyObject* depth_val = Py_BuildValue("(f)", depth[i]);
      PyList_SetItem(depth_list, i, depth_val);
   }
   glfwTerminate();
   free(pixels);
   free(depth);
   PyObject *ret_val = PyList_New(2);
   PyList_SetItem(ret_val, 0, pixel_list);
   PyList_SetItem(ret_val, 1, depth_list);
   return ret_val;
}

static PyObject* glCreateBuffer(PyObject* self, PyObject* args) {
   GLuint width, height;
   GLchar* image_path;
   if (!PyArg_ParseTuple(args, "sii", &image_path, &width, &height))
      return NULL;

   createTextures(image_path);


   glGenVertexArrays(1, &vertexArrayID);
   glBindVertexArray(vertexArrayID);
   
   static const GLfloat vertices[] = {
      p,  p, 0.0f,   1.0f, 1.0f,  // top right
      p, -p, 0.0f,   1.0f, 0.0f,  // bottom right
      -p, -p, 0.0f,   0.0f, 0.0f,  // bottom left
      -p,  p, 0.0f,   0.0f, 1.0f   // top left 
   };
   GLuint indices[] = {  // note that we start from 0!
      0, 1, 3,   // first triangle
      1, 2, 3    // second triangle
   };  

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   unsigned int EBO;
   glGenBuffers(1, &EBO);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 
   
   return Py_BuildValue("(iii)", vertexBuffer, EBO, vertexArrayID);
}

static PyObject* glCreateTexture(PyObject* self, PyObject* args) {
   GLuint width, height;
   GLuint fb, color, depth;
   if (!PyArg_ParseTuple(args, "ii", &width, &height))
      return NULL;
   
   glGenFramebuffers(1, &fb);
   glGenTextures(1, &color);
   glGenRenderbuffers(1, &depth);

   glBindFramebuffer(GL_FRAMEBUFFER, fb);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, color);
   glTexImage2D(   GL_TEXTURE_2D, 
         0, 
         GL_RGBA, 
         width, height,
         0, 
         GL_RGBA, 
         GL_FLOAT, 
         NULL);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

   //glBindRenderbuffer(GL_RENDERBUFFER, depth);
   //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
   //glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
   GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
   glDrawBuffers(1, DrawBuffers); 

   GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
   status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER); 
   switch(status) {
      case GL_FRAMEBUFFER_COMPLETE:
         break;
      case GL_FRAMEBUFFER_UNSUPPORTED:
         break;
      default:
         fputs("Framebuffer Error\n", stderr);
         printf("%d\n", glGetError());
   }
   glBindTexture(GL_TEXTURE_2D, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   return Py_BuildValue("(ii)", color, fb);
}


static PyObject* glDraw(PyObject* self, PyObject* args) {
   PyObject *float_list, *eye, *target, *up, *vbo, *ebo, *vid, *dimension, *timeObj, *custom_uniforms;
   int pr_length;
   GLfloat *pr;
   GLuint vertexBuffer, EBO, width, height, vertexArrayID, maxMarchingSteps;
   GLfloat time, minDist, maxDist, epsilon;
   GLfloat eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ;
   
   PyObject *dict;
   if (!PyArg_ParseTuple(args, "O", &dict)) {
      return NULL;
   }
   float_list = PyDict_GetItemString(dict,"matrix");
   eye = PyDict_GetItemString(dict, "eye");
   target = PyDict_GetItemString(dict, "target");
   up = PyDict_GetItemString(dict, "up");
   vbo = PyDict_GetItemString(dict, "vbo");
   ebo = PyDict_GetItemString(dict, "ebo");
   vid = PyDict_GetItemString(dict, "vid");
   dimension = PyDict_GetItemString(dict, "dimension");
   timeObj = PyDict_GetItemString(dict, "time");
   custom_uniforms = PyDict_GetItemString(dict, "custom_uniforms");

   maxMarchingSteps = PyLong_AsLong(PyDict_GetItemString(dict, "max_marching_steps"));
   minDist = PyFloat_AsDouble(PyDict_GetItemString(dict, "min_dist"));
   maxDist = PyFloat_AsDouble(PyDict_GetItemString(dict, "max_dist"));
   epsilon = PyFloat_AsDouble(PyDict_GetItemString(dict, "epsilon"));

   time = PyFloat_AsDouble(timeObj);

   eyeX = PyFloat_AsDouble(PyList_GetItem(eye, 0));
   eyeY = PyFloat_AsDouble(PyList_GetItem(eye, 1));
   eyeZ = PyFloat_AsDouble(PyList_GetItem(eye, 2));
   
   targetX = PyFloat_AsDouble(PyList_GetItem(target, 0));
   targetY = PyFloat_AsDouble(PyList_GetItem(target, 1));
   targetZ = PyFloat_AsDouble(PyList_GetItem(target, 2));
   
   upX = PyFloat_AsDouble(PyList_GetItem(up, 0));
   upY = PyFloat_AsDouble(PyList_GetItem(up, 1));
   upZ = PyFloat_AsDouble(PyList_GetItem(up, 2));
   
   width = PyLong_AsLong(PyList_GetItem(dimension, 0));
   height = PyLong_AsLong(PyList_GetItem(dimension, 1));

   vertexBuffer = PyLong_AsLong(vbo);
   EBO = PyLong_AsLong(ebo);
   vertexArrayID = PyLong_AsLong(vid);

   pr_length = PyObject_Length(float_list);
   if (pr_length < 0)
      return NULL;
   pr = (GLfloat *) malloc(sizeof(GLfloat *) * pr_length);
   if (pr == NULL)
      return NULL;
   for (int index = 0; index < pr_length; index++) {
      PyObject *item;
      item = PyList_GetItem(float_list, index);
      if (!PyFloat_Check(item))
         pr[index] = 0.0;
      pr[index] = PyFloat_AsDouble(item);
   }

   //Get custom uniforms
   pr_length = PyObject_Length(custom_uniforms);
   for(int i = 0; i < pr_length; ++i) {
      PyObject *item = PyList_GetItem(custom_uniforms, i);
      PyObject *uniform_variable = PyDict_GetItemString(item, "key");
      PyObject *uniform_value = PyDict_GetItemString(item, "value");
      PyObject *uniform_type = PyDict_GetItemString(item, "type");
      const char* type = PyBytes_AsString(PyUnicode_AsUTF8String(uniform_type));
      const char* key = PyBytes_AsString(PyUnicode_AsUTF8String(uniform_variable));

      unsigned int loc = glGetUniformLocation(programID, key);
      if (strcmp(type, "1f") == 0) {
         GLfloat value = PyFloat_AsDouble(uniform_value);
         glUniform1f(loc, value);
      } 
      else if (strcmp(type, "2f") == 0) {
         glUniform2f(loc, PyFloat_AsDouble(PyList_GetItem(uniform_value, 0)), 
                           PyFloat_AsDouble(PyList_GetItem(uniform_value, 1)));
      }
      else if (strcmp(type, "3f") == 0) {
         glUniform3f(loc, PyFloat_AsDouble(PyList_GetItem(uniform_value, 0)), 
                           PyFloat_AsDouble(PyList_GetItem(uniform_value, 1)),
                           PyFloat_AsDouble(PyList_GetItem(uniform_value, 1)));
      }else if (strcmp(type, "1i") == 0) {
         GLfloat value = PyLong_AsLong(uniform_value);
         glUniform1i(loc, value);
      }
   }

   glViewport(0,0, width, height);

   ///glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
   glEnable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);  
   glDepthFunc(GL_LESS);  
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glBindTexture(GL_TEXTURE_2D, channel_0);

   unsigned int maxMarchingStepsLoc = glGetUniformLocation(programID, "MAX_MARCHING_STEPS");
   glUniform1i(maxMarchingStepsLoc, maxMarchingSteps);

   unsigned int minDistLoc = glGetUniformLocation(programID, "MIN_DIST");
   glUniform1f(minDistLoc, minDist);

   unsigned int maxDistLoc = glGetUniformLocation(programID, "MAX_DIST");
   glUniform1f(maxDistLoc, maxDist);

   unsigned int epsilonLoc = glGetUniformLocation(programID, "EPSILON");
   glUniform1f(epsilonLoc, epsilon);

   unsigned int viewProjectionLoc = glGetUniformLocation(programID, "modelViewProjectionMatrix");
   glUniformMatrix4fv(viewProjectionLoc, 1, GL_FALSE, pr);
   
   unsigned int cameraMatrixLoc = glGetUniformLocation(programID, "tCameraMatrix");
   glUniformMatrix4fv(cameraMatrixLoc, 1, GL_FALSE, pr);

   unsigned int resolutionLoc = glGetUniformLocation(programID, "iResolution");
   glUniform3f(resolutionLoc, width, height, 1.0f);

   unsigned int eyeLoc = glGetUniformLocation(programID, "eye");
   glUniform3f(eyeLoc, eyeX, eyeY, eyeZ);

   unsigned int targetLoc = glGetUniformLocation(programID, "target");
   glUniform3f(targetLoc, targetX, targetY, targetZ);

   unsigned int upLoc = glGetUniformLocation(programID, "up");
   glUniform3f(upLoc, upX, upY, upZ);

   unsigned int timeLoc = glGetUniformLocation(programID, "iTime");
   glUniform1f(timeLoc, time);

   unsigned int channelLoc = glGetUniformLocation(programID, "channel");
   glUniform1i(channelLoc, 0);


   glBindVertexArray(vertexArrayID);
   // 1rst attribute buffer : vertices
   glEnableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
   glVertexAttribPointer(
      0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
      3,                  // size
      GL_FLOAT,           // type
      GL_FALSE,           // normalized?
      5 * sizeof(GLfloat),                  // stride
      (void*)0            // array buffer offset
   );

   glEnableVertexAttribArray(1); 
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
   

   // Draw the triangle !
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);  
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);
   //glDisable(GL_BLEND);
   //glDisable(GL_DEPTH_TEST);  
   
   return Py_None;
}


static PyMethodDef methods[] = {
    {"glCompileProgram", glCompileProgram, METH_VARARGS, "Init opengl context"},
    {"glDraw", glDraw, METH_VARARGS, "Draw Opengl"},
    {"glRenderInit", glRenderInit, METH_VARARGS, "Render initialisation"},
    {"glRenderResult", glRenderResult, METH_VARARGS, "Render end"},
    {"glUseProgram", glProgram, METH_VARARGS, "Use shader program"},
    {"glCreateBuffers", glCreateBuffer, METH_VARARGS, "Create buffers"},
    {"glCreateTexture", glCreateTexture, METH_VARARGS, "Create texture channel"},
    { NULL, NULL, 0, NULL }
};

// Our Module Definition struct
static struct PyModuleDef sdf_engine = {
    PyModuleDef_HEAD_INIT,
    "sdf_engine",
    "SDF Render Engine Module",
    -1,
    methods
};

// Initializes our module using our above struct
PyMODINIT_FUNC PyInit_sdf_engine(void)
{
    return PyModule_Create(&sdf_engine);
}
