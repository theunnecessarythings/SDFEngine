import bpy
import gpu
import bgl
from mathutils import Matrix, Vector
from gpu_extras.batch import batch_for_shader
from gpu_extras.presets import draw_circle_2d

import imbuf

import numpy as np
import sys

import sdf_engine


MAX_MARCHING_STEPS = 255
MIN_DIST = 0.0
MAX_DIST = 100.0
EPSILON = 0.0001

''' TODO resizing area error fix'''
class SDFRenderEngine(bpy.types.RenderEngine):
    # These three members are used by blender to set up the
    # RenderEngine; define its internal name, visible name and capabilities.
    bl_idname = "SDF_ENGINE"
    bl_label = "SDF Engine"
    bl_use_preview = True

    # Init is called whenever a new render engine instance is created. Multiple
    # instances may exist at the same time, for example for a viewport and final
    # render.
    def __init__(self):
        self.scene_data = None
        self.draw_data = None

    # When the render engine instance is destroy, this is called. Clean up any
    # render engine data here, for example stopping running render threads.
    def __del__(self):
        pass
    
    def update_render_passes(self, scene=None, renderlayer=None):
        self.register_pass(scene, renderlayer, "Combined", 4, "RGBA", 'COLOR')
        self.register_pass(scene, renderlayer, "Depth", 1, "X", 'VALUE')
    
    # This is the method called by Blender for both final renders (F12) and
    # small preview for materials, world and lights.
    def render(self, depsgraph):
        scene = depsgraph.scene
        scale = scene.render.resolution_percentage / 100.0
        self.size_x = int(scene.render.resolution_x * scale)
        self.size_y = int(scene.render.resolution_y * scale)
        self.is_rendering = True
        # Fill the render result with a flat color. The framebuffer is
        # defined as a list of pixels, each pixel itself being a list of
        # R,G,B,A values.
        if self.is_preview:
            color = [0.1, 0.2, 0.1, 1.0]
        else:
            color = [0.2, 0.1, 0.1, 1.0]

        pixel_count = self.size_x * self.size_y
        rect = [color] * pixel_count
        
        m = scene.camera.matrix_world.inverted()
        m = np.array(m).flatten('F').tolist()
        
        eye = scene.camera.location
        up = scene.camera.matrix_world.to_quaternion() @ Vector((0.0, 1.0, 0.0))
        target = scene.camera.matrix_world.to_quaternion() @ Vector((0.0, 0.0, -1.0))
        time = scene.frame_current/24
        
        vertex_shader, fragment_shader = get_shaders()
        
        sdf_engine.glRenderInit(self.size_x, self.size_y)
        shader = sdf_engine.glCompileProgram(vertex_shader, fragment_shader)
        sdf_engine.glUseProgram(shader)
        image_path = '/home/night-queen/Videos/Youtube/noise_medium.png'
        vb, ebo, vid = sdf_engine.glCreateBuffers(image_path, self.size_x, self.size_y)
        
        args = {"dimension": [self.size_x, self.size_y], "eye": list(eye), "target": list(target), "up": list(up), "vbo": vb, "ebo": ebo, "matrix": m, "time": time, "vid": vid, "max_marching_steps": MAX_MARCHING_STEPS, "min_dist": MIN_DIST, "max_dist": MAX_DIST, "epsilon": EPSILON, "custom_uniforms":[{'key':'some','type':'1f','value':1.0}]}
        sdf_engine.glDraw(args)

        
        
        #TODO try parallelizing later
        pixels, depth = sdf_engine.glRenderResult(self.size_x, self.size_y)        
            
        # Here we write the pixel values to the RenderResult
        result = self.begin_result(0, 0, self.size_x, self.size_y)
        layer = result.layers[0].passes["Combined"]
        layer.rect = pixels
        layer = result.layers[0].passes["Depth"]
        layer.rect = depth
        self.end_result(result)

    # For viewport renders, this method gets called once at the start and
    # whenever the scene or 3D viewport changes. This method is where data
    # should be read from Blender in the same thread. Typically a render
    # thread will be started to do the work while keeping Blender responsive.
    def view_update(self, context, depsgraph):
        region = context.region
        view3d = context.space_data
        scene = depsgraph.scene

        # Get viewport dimensions
        dimensions = region.width, region.height
        #self.draw_data = EngineDrawData(dimensions)

        if not self.scene_data:
            # First time initialization
            self.scene_data = []
            first_time = True

            # Loop over all datablocks used in the scene.
            for datablock in depsgraph.ids:
                pass
        else:
            first_time = False

            # Test which datablocks changed
            for update in depsgraph.updates:
                print("Datablock updated: ", update.id.name)

            # Test if any material was added, removed or changed.
            if depsgraph.id_type_updated('MATERIAL'):
                print("Materials updated")

        # Loop over all object instances in the scene.
        if first_time or depsgraph.id_type_updated('OBJECT'):
            for instance in depsgraph.object_instances:
                pass

    # For viewport renders, this method is called whenever Blender redraws
    # the 3D viewport. The renderer is expected to quickly draw the render
    # with OpenGL, and not perform other expensive work.
    # Blender will draw overlays for selection and editing on top of the
    # rendered image automatically.
    def view_draw(self, context, depsgraph):
        region = context.region
        scene = depsgraph.scene

        # Get viewport dimensions
        dimensions = region.width, region.height
        if not self.draw_data or self.draw_data.dimensions != dimensions:
            #self.is_rendering = False
            self.draw_data = EngineDrawData(dimensions)
        self.draw_data.draw()
    
    



class EngineDrawData:
    def __init__(self, dimensions):
        self.dimensions = dimensions
        image_path = '/home/night-queen/Videos/Youtube/noise_medium.png'
        vertex_shader, fragment_shader = get_shaders()
        self.program = sdf_engine.glCompileProgram(vertex_shader, fragment_shader)
        self.vb, self.ebo, self.vid = sdf_engine.glCreateBuffers(image_path, self.dimensions[0], self.dimensions[1])  
        

    def draw(self):
        region = bpy.context.region_data
        target = region.view_location
        view_projection_matrix = region.perspective_matrix
        view_rotation = region.view_rotation
        
        eye = region.view_matrix.inverted().translation
        up = region.view_rotation @ Vector((0.0, 1.0, 0.0))
        time = bpy.context.scene.frame_current/24
        
        m = np.array(view_projection_matrix).flatten('F').tolist()
        
        
        sdf_engine.glUseProgram(self.program)
        
        min_dist = bpy.context.scene.sdf.min_dist
        max_dist = bpy.context.scene.sdf.max_dist
        epsilon = bpy.context.scene.sdf.epsilon
        max_marching_steps = bpy.context.scene.sdf.max_marching_steps
        
        args = {"dimension": list(self.dimensions), "eye": list(eye), "target": list(target), "up": list(up), "vbo": self.vb, "ebo": self.ebo, "matrix": m, "time": time, "vid": self.vid, "max_marching_steps": max_marching_steps, "min_dist": min_dist, "max_dist": max_dist, "epsilon": epsilon, "custom_uniforms":[{'key':'some','type':'1f','value':1.0}]}
        sdf_engine.glDraw(args)
        

def get_shaders():
    vertex_shader = bpy.data.texts['shader_sdf.vert'].as_string()
    try:
        scene_sdf = bpy.context.scene.sdf.shader_program.as_string()
    except:
        scene_sdf = ''
    uniforms = bpy.data.texts['uniforms.frag'].as_string()
    if bpy.context.scene.sdf.template_type == 'DEFAULT':
        library = bpy.data.texts['library.frag'].as_string()
        main = bpy.data.texts['main.frag'].as_string()
    else:
        library = main = ''
    fragment_shader = uniforms + library + scene_sdf + main
    return vertex_shader, fragment_shader




template_items = [
    ("DEFAULT", "Default", "", 1),
    ("UNIFORM_ONLY", "Uniforms Only", "", 2),
]

def update_shader(self, context):
    for area in context.screen.areas:
        if area.type == 'VIEW_3D':
            space = area.spaces[0]
            break
    space.shading.type = 'SOLID'
    space.shading.type = 'MATERIAL'

class SDFProperties(bpy.types.PropertyGroup):
    shader_program: bpy.props.PointerProperty(name='Shader Program', type=bpy.types.Text, update=update_shader)
    template_type: bpy.props.EnumProperty(name='Template', items=template_items, update=update_shader)
    max_marching_steps: bpy.props.IntProperty(name='Max Marching Steps', min=1, default=255)
    min_dist: bpy.props.FloatProperty(name='Min Distance', default=0.0, min=0)
    max_dist: bpy.props.FloatProperty(name='Max Distance', default=100.0, min=0)
    epsilon: bpy.props.FloatProperty(name='Epsilon', default=0.0001, min=0)
    channel_0: bpy.props.CollectionProperty(name='Channel 0', type=bpy.types.OperatorFileListElement)

class SDFEnginePanel(bpy.types.Panel):
    """SDF Engine"""
    bl_label = "SDF Engine"
    bl_idname = "OBJECT_PT_sdf_engine"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "render"
    COMPAT_ENGINES = {'SDF_ENGINE'}
    
    def draw(self, context):
        layout = self.layout
        props = context.scene.sdf
        split = layout.split()
        col = split.column()
        col.label(text='Template')
        col.label(text='Shader Program')
        col.label(text='Max Marching Steps')
        col.label(text='Min Distance')
        col.label(text='Max Distance')
        col.label(text='Epsilon')
        col = split.column()
        col.prop(props, 'template_type', text='')
        col.prop(props, 'shader_program', text='')
        col.prop(props, 'max_marching_steps', text='')
        col.prop(props, 'min_dist', text='')
        col.prop(props, 'max_dist', text='')
        col.prop(props, 'epsilon', text='')
        col.prop(props, 'channel_0', text='')
    @classmethod
    def poll(cls, context):
        return context.engine in cls.COMPAT_ENGINES


# RenderEngines also need to tell UI Panels that they are compatible with.
# We recommend to enable all panels marked as BLENDER_RENDER, and then
# exclude any panels that are replaced by custom panels registered by the
# render engine, or that are not supported.
def get_panels():
    exclude_panels = {
        'VIEWLAYER_PT_filter',
        'VIEWLAYER_PT_layer_passes',
    }

    panels = []
    for panel in bpy.types.Panel.__subclasses__():
        if hasattr(panel, 'COMPAT_ENGINES') and 'BLENDER_RENDER' in panel.COMPAT_ENGINES:
            if panel.__name__ not in exclude_panels:
                panels.append(panel)

    return panels

classes = [SDFRenderEngine, SDFEnginePanel, SDFProperties]

def register():
    # Register the RenderEngine
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.sdf = bpy.props.PointerProperty(type=SDFProperties)

    for panel in get_panels():
        panel.COMPAT_ENGINES.add('SDF_ENGINE')

def unregister():
    for cls in classes:
        bpy.utils.unregister_class(cls)

    for panel in get_panels():
        if 'SDF_ENGINE' in panel.COMPAT_ENGINES:
            panel.COMPAT_ENGINES.remove('SDF_ENGINE')


if __name__ == "__main__":
    register()
