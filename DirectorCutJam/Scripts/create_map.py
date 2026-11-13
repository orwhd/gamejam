"""Run once in Unreal Editor Python, or UnrealEditor-Cmd -run=pythonscript.
Creates actual .umap and material assets; never fabricates asset binaries.
Safe to rerun: refuses to overwrite the map unless -DCRebuildMap is supplied.
"""
import unreal
import os

MAP = '/Game/Maps/Firefighter'
if unreal.EditorAssetLibrary.does_asset_exist(MAP) and '-DCRebuildMap' not in unreal.SystemLibrary.get_command_line():
    unreal.log('DC map already exists; leaving authored map untouched.')
else:
    colors = {
        'Ground': (0.12, 0.16, 0.19), 'Floor': (0.25, 0.28, 0.29),
        'Wall': (0.32, 0.37, 0.4), 'Fire': (1.0, 0.18, 0.015),
        'Victim': (0.05, 0.8, 0.6), 'Captain': (0.16, 0.35, 0.9),
        'Player': (1.0, 0.65, 0.05), 'Entry': (0.85, 0.52, 0.1),
    }
    materials = {}
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, rgb in colors.items():
        path = '/Game/Materials/M_' + name
        mat = unreal.load_asset(path)
        if not mat:
            mat = tools.create_asset('M_' + name, '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())
            node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
            node.set_editor_property('constant', unreal.LinearColor(*rgb, 1.0))
            unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
            unreal.MaterialEditingLibrary.recompile_material(mat)
            unreal.EditorAssetLibrary.save_loaded_asset(mat)
        materials[name] = mat

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.new_level(MAP)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    director_class = unreal.load_class(None, '/Script/DirectorCutJam.DCDirector')
    director = actors.spawn_actor_from_class(director_class, unreal.Vector())
    director.set_actor_label('DC_TestScene_Adjust_Placeholders_Here')
    for comp in director.get_components_by_class(unreal.StaticMeshComponent):
        name = comp.get_name()
        key = {'Ground': 'Ground', 'InteriorFloor': 'Floor', 'Fire': 'Fire',
               'Victim': 'Victim', 'Captain': 'Captain', 'EntryStripe': 'Entry'}.get(name, 'Wall')
        comp.set_material(0, materials[key])
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-450, 0, 100))
    start.set_actor_label('Firefighter_Start')
    light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,1000), unreal.Rotator(-55,-30,0))
    light.light_component.set_editor_property('intensity', 4.0)
    light.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,900))
    sky.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('intensity', 1.0)
    sky.light_component.set_editor_property('source_type', unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    cube = unreal.load_asset('/Engine/EngineResources/DefaultTextureCube.DefaultTextureCube')
    if cube:
        sky.light_component.set_editor_property('cubemap', cube)
    game_mode = unreal.load_class(None, '/Script/DirectorCutJam.DCGameMode')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', game_mode)
    levels.save_current_level()
    unreal.EditorAssetLibrary.save_directory('/Game')
    unreal.log('DC_MAP_CREATED ' + MAP)
