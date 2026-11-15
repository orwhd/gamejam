"""Run in Unreal Editor Python or UnrealEditor-Cmd -ExecutePythonScript=...

Imports generated Stage 2 placeholder sprites/environment textures/WAV SFX,
creates .uasset materials, material instances, DataAssets, and the /Game/Maps/Firefighter .umap.
Safe to rerun: refuses to overwrite an existing map unless -DCRebuildMap is passed on the command line.
"""

import os
import unreal


MAP_PATH = '/Game/Maps/Firefighter'
MATERIALS_DIR = '/Game/Materials'
DATA_DIR = '/Game/Data'
TEX_CHAR_DIR = '/Game/Textures/Characters'
TEX_INTERACT_DIR = '/Game/Textures/Interactions'
TEX_ENV_DIR = '/Game/Textures/Environments'
AUDIO_DIR = '/Game/Audio'

WRAP_TEXTURES = {'T_Wall_VintageGreen', 'T_Floor_WarmWoven', 'T_Porch_Wood'}
RAW_CHARACTER_FILES_TO_SKIP = {
    'firefighter.png',
    'witch.png',
    'rescue.png',
    'little girl.png',
    'dog.png',
}


def _find_enum_value(enum_cls, target_name):
    norm_target = target_name.replace('_', '').lower()
    for attr in dir(enum_cls):
        if attr.replace('_', '').lower() == norm_target:
            return getattr(enum_cls, attr)
    raise RuntimeError(f'Enum value {target_name} not found on {enum_cls}')


def _get_repo_assets_root():
    scripts_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(scripts_dir, '..', '..', 'assets'))


def _load_or_create_asset(asset_name, package_path, asset_class, factory):
    full_path = f'{package_path}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        existing = unreal.load_asset(full_path)
        if existing is not None:
            return existing
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    return tools.create_asset(asset_name, package_path, asset_class, factory)


def _set_first_valid_editor_property(obj, prop_names, value):
    for prop_name in prop_names:
        try:
            obj.set_editor_property(prop_name, value)
            return prop_name
        except Exception:
            continue
    raise RuntimeError(f'None of properties {prop_names} exist on {obj.get_name()}')


def _connect(mat_lib, src_expr, src_pin, dst_expr, dst_pin):
    ok = mat_lib.connect_material_expressions(src_expr, src_pin, dst_expr, dst_pin)
    if not ok:
        raise RuntimeError(
            f'Failed to connect {src_expr.get_name()}.{src_pin} -> {dst_expr.get_name()}.{dst_pin}'
        )


def _connect_prop(mat_lib, src_expr, src_pin, mat_prop):
    ok = mat_lib.connect_material_property(src_expr, src_pin, mat_prop)
    if not ok:
        raise RuntimeError(f'Failed to connect {src_expr.get_name()}.{src_pin} -> {mat_prop}')


def _import_assets():
    assets_root = _get_repo_assets_root()
    import_groups = [
        (os.path.join(assets_root, 'characters'), TEX_CHAR_DIR, '.png', 'texture'),
        (os.path.join(assets_root, 'envirments-character-interactions'), TEX_INTERACT_DIR, '.png', 'texture'),
        (os.path.join(assets_root, 'envirments'), TEX_ENV_DIR, '.png', 'texture'),
        (os.path.join(assets_root, 'audio'), AUDIO_DIR, '.wav', 'audio'),
    ]

    specs = []
    for folder, dest_path, ext, kind in import_groups:
        if not os.path.isdir(folder):
            raise RuntimeError(f'Missing source asset folder: {folder}')
        is_char_folder = (dest_path == TEX_CHAR_DIR)
        for filename in sorted(os.listdir(folder)):
            if not filename.lower().endswith(ext):
                continue
            if is_char_folder and filename.lower() in RAW_CHARACTER_FILES_TO_SKIP:
                unreal.log(f'Skipping raw artist character file: {filename}')
                continue
            abs_path = os.path.join(folder, filename)
            if not os.path.isfile(abs_path):
                continue
            asset_name = os.path.splitext(filename)[0]
            specs.append((abs_path, dest_path, asset_name, kind))

    if not specs:
        raise RuntimeError('No assets discovered to import.')

    tasks = []
    for abs_path, dest_path, asset_name, _ in specs:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', abs_path)
        task.set_editor_property('destination_path', dest_path)
        task.set_editor_property('destination_name', asset_name)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('replace_existing_settings', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        tasks.append(task)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_tools.import_asset_tasks(tasks)

    textures = {}
    sounds = {}
    for _, dest_path, asset_name, kind in specs:
        full_path = f'{dest_path}/{asset_name}'
        asset = unreal.load_asset(full_path)
        if asset is None:
            raise RuntimeError(f'Failed to load imported asset: {full_path}')

        if kind == 'texture':
            wrap_mode = (
                unreal.TextureAddress.TA_WRAP
                if asset_name in WRAP_TEXTURES
                else unreal.TextureAddress.TA_CLAMP
            )
            asset.set_editor_property('address_x', wrap_mode)
            asset.set_editor_property('address_y', wrap_mode)
            asset.set_editor_property('srgb', True)
            asset.set_editor_property('filter', unreal.TextureFilter.TF_BILINEAR)
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
            textures[asset_name] = asset
        else:
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
            sounds[asset_name] = asset

    required_textures = [
        'T_UI_AngelDogSticker',
        'T_UI_WitchHatSticker',
        'T_Fire_Stage4_0',
        'T_Fire_Stage4_1',
        'T_Fire_Stage4_2',
        'T_Civilian_Rescued_0',
        'T_Civilian_Rescued_1',
        'T_MatchGirl_Idle_0',
        'T_MatchGirl_Idle_1',
        'T_AngelDog_Idle_0',
        'T_AngelDog_Idle_1',
        'T_AngelDog_Happy_0',
        'T_AngelDog_Happy_1',
        'T_WaterJet_0',
        'T_WaterJet_1',
        'T_SafeZone_Mat',
        'T_Porch_Wood',
        'T_Portrait_Coco',
    ]
    missing_tex = [name for name in required_textures if name not in textures]
    if missing_tex:
        raise RuntimeError(f'Missing required v3.1 textures: {missing_tex}')
    if 'BGM_StageLoop' not in sounds:
        raise RuntimeError('Missing required v3.1 BGM sound: BGM_StageLoop')

    unreal.EditorAssetLibrary.save_directory('/Game/Textures')
    unreal.EditorAssetLibrary.save_directory('/Game/Audio')
    unreal.log(f'DC_ASSETS_IMPORTED textures={len(textures)} sounds={len(sounds)}')
    return textures, sounds


def _detect_plane_uv_axes():
    """Inspect /Engine/BasicShapes/Plane UVs for cards rotated FRotator(-90, 0, 0) facing -X.

    Under FRotator(-90, 0, 0) inside PaperVisual (Yaw ~ 180 deg):
    - Local +Y stays World -Y (Camera LEFT); local -Y stays World +Y (Camera RIGHT).
    - Local -X rotates to World +Z (UP); local +X rotates to World -Z (DOWN).
    So desired upright image coordinates satisfy:
    - target_u = 0 at py=+50 (Camera LEFT), 1 at py=-50 (Camera RIGHT)
    - target_v = 0 at px=-50 (TOP +Z),      1 at px=+50 (BOTTOM -Z)
    """
    plane = unreal.load_asset('/Engine/BasicShapes/Plane.Plane')
    default_axes = {
        'u_channel': 'r',
        'u_invert': False,
        'v_channel': 'g',
        'v_invert': False,
    }
    if plane is None or not hasattr(plane, 'get_static_mesh_description'):
        unreal.log_warning('BasicShapes/Plane unavailable for UV inspection; using identity UV mapping.')
        return default_axes

    desc = plane.get_static_mesh_description(0)
    if desc is None:
        unreal.log_warning('StaticMeshDescription unavailable on Plane; using identity UV mapping.')
        return default_axes

    samples = []
    vi_count = desc.get_vertex_instance_count()
    for i in range(vi_count):
        vi = unreal.VertexInstanceID(i)
        if not desc.is_vertex_instance_valid(vi):
            continue
        vid = desc.get_vertex_instance_vertex(vi)
        pos = desc.get_vertex_position(vid)
        uv = desc.get_vertex_instance_uv(vi, 0)
        samples.append((float(pos.x), float(pos.y), float(uv.x), float(uv.y)))

    if not samples:
        unreal.log_warning('No vertex instances found on BasicShapes/Plane; using identity UV mapping.')
        return default_axes

    for px, py, u_raw, v_raw in samples:
        unreal.log(f'DC_PLANE_UV pos=({px:.1f}, {py:.1f}) uv=({u_raw:.3f}, {v_raw:.3f})')

    def _match_target(target_fn):
        for ch, getter in (('r', lambda s: s[2]), ('g', lambda s: s[3])):
            for inv in (False, True):
                ok = True
                for px, py, u_raw, v_raw in samples:
                    val = getter((px, py, u_raw, v_raw))
                    mapped = (1.0 - val) if inv else val
                    if abs(mapped - target_fn(px, py)) > 0.05:
                        ok = False
                        break
                if ok:
                    return ch, inv
        return None

    u_match = _match_target(lambda px, py: 0.5 - (py / 100.0))
    v_match = _match_target(lambda px, py: 0.5 + (px / 100.0))
    if u_match is None or v_match is None:
        unreal.log_warning(
            f'Could not uniquely solve Plane UV axes (u={u_match}, v={v_match}); using default.'
        )
        return default_axes

    axes = {
        'u_channel': u_match[0],
        'u_invert': u_match[1],
        'v_channel': v_match[0],
        'v_invert': v_match[1],
    }
    unreal.log(f'DC_PLANE_UV_SOLVED {axes}')
    return axes


def _build_plane_uv_nodes(mat, mat_lib, uv_axes, x_base, y_base, flip_u_expr=None):
    tex_coord = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionTextureCoordinate, x_base - 960, y_base
    )
    tex_coord.set_editor_property('coordinate_index', 0)

    def _extract_axis(channel, invert, row_y):
        mask = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionComponentMask, x_base - 760, row_y
        )
        mask.set_editor_property('r', channel == 'r')
        mask.set_editor_property('g', channel == 'g')
        mask.set_editor_property('b', False)
        mask.set_editor_property('a', False)
        _connect(mat_lib, tex_coord, '', mask, '')
        if invert:
            one_minus = mat_lib.create_material_expression(
                mat, unreal.MaterialExpressionOneMinus, x_base - 580, row_y
            )
            _connect(mat_lib, mask, '', one_minus, '')
            return one_minus
        return mask

    u_expr = _extract_axis(uv_axes['u_channel'], uv_axes['u_invert'], y_base - 60)
    v_expr = _extract_axis(uv_axes['v_channel'], uv_axes['v_invert'], y_base + 100)

    if flip_u_expr is not None:
        half_node = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionConstant, x_base - 580, y_base - 170
        )
        half_node.set_editor_property('r', 0.5)

        sub_node = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionSubtract, x_base - 400, y_base - 80
        )
        _connect(mat_lib, u_expr, '', sub_node, 'A')
        _connect(mat_lib, half_node, '', sub_node, 'B')

        mul_node = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionMultiply, x_base - 240, y_base - 80
        )
        _connect(mat_lib, sub_node, '', mul_node, 'A')
        _connect(mat_lib, flip_u_expr, '', mul_node, 'B')

        add_node = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionAdd, x_base - 90, y_base - 80
        )
        _connect(mat_lib, mul_node, '', add_node, 'A')
        _connect(mat_lib, half_node, '', add_node, 'B')
        u_expr = add_node

    append_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionAppendVector, x_base + 80, y_base
    )
    _connect(mat_lib, u_expr, '', append_node, 'A')
    _connect(mat_lib, v_expr, '', append_node, 'B')
    return append_node


def _create_paper_masked_material(uv_axes):
    mat = _load_or_create_asset(
        'M_PaperMasked',
        MATERIALS_DIR,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    mat_lib = unreal.MaterialEditingLibrary
    mat_lib.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('opacity_mask_clip_value', 0.333)

    flip_u_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionScalarParameter, -900, 120
    )
    flip_u_node.set_editor_property('parameter_name', 'FlipU')
    flip_u_node.set_editor_property('default_value', 1.0)

    uv_node = _build_plane_uv_nodes(
        mat, mat_lib, uv_axes, x_base=-420, y_base=-120, flip_u_expr=flip_u_node
    )

    white_tex = unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture')
    tex_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionTextureSampleParameter2D, -180, -160
    )
    tex_node.set_editor_property('parameter_name', 'PaperTexture')
    if white_tex is not None:
        tex_node.set_editor_property('texture', white_tex)
    tex_node.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    _connect(mat_lib, uv_node, '', tex_node, 'UVs')

    tint_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionVectorParameter, -180, 100
    )
    tint_node.set_editor_property('parameter_name', 'TintColor')
    tint_node.set_editor_property('default_value', unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    base_mul = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, 80, -120
    )
    _connect(mat_lib, tex_node, '', base_mul, 'A')
    _connect(mat_lib, tint_node, '', base_mul, 'B')
    _connect_prop(mat_lib, base_mul, '', unreal.MaterialProperty.MP_BASE_COLOR)

    mask_mul = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, 80, 120
    )
    _connect(mat_lib, tex_node, 'A', mask_mul, 'A')
    _connect(mat_lib, tint_node, 'A', mask_mul, 'B')
    _connect_prop(mat_lib, mask_mul, '', unreal.MaterialProperty.MP_OPACITY_MASK)

    rough_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant, 80, 260
    )
    rough_node.set_editor_property('r', 0.88)
    _connect_prop(mat_lib, rough_node, '', unreal.MaterialProperty.MP_ROUGHNESS)

    mat_lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def _create_paper_material_instance(name, parent_mat, rgba):
    mi = _load_or_create_asset(
        name,
        MATERIALS_DIR,
        unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew(),
    )
    mat_lib = unreal.MaterialEditingLibrary
    mat_lib.set_material_instance_parent(mi, parent_mat)
    mat_lib.set_material_instance_vector_parameter_value(
        mi, 'TintColor', unreal.LinearColor(*rgba)
    )
    mat_lib.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi


def _create_solid_material(name, rgb, roughness=0.85, emissive_rgb=None):
    mat = _load_or_create_asset(
        name,
        MATERIALS_DIR,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    mat_lib = unreal.MaterialEditingLibrary
    mat_lib.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided', False)

    color_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, -300, -60
    )
    color_node.set_editor_property('constant', unreal.LinearColor(*rgb, 1.0))
    _connect_prop(mat_lib, color_node, '', unreal.MaterialProperty.MP_BASE_COLOR)

    rough_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -300, 100
    )
    rough_node.set_editor_property('r', roughness)
    _connect_prop(mat_lib, rough_node, '', unreal.MaterialProperty.MP_ROUGHNESS)

    if emissive_rgb is not None:
        emissive_node = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionConstant3Vector, -300, 220
        )
        emissive_node.set_editor_property('constant', unreal.LinearColor(*emissive_rgb, 1.0))
        _connect_prop(mat_lib, emissive_node, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def _create_textured_material(
    name,
    texture_asset,
    u_tiling=1.0,
    v_tiling=1.0,
    roughness=0.85,
    emissive_scale=0.0,
    uv_axes=None,
):
    mat = _load_or_create_asset(
        name,
        MATERIALS_DIR,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    mat_lib = unreal.MaterialEditingLibrary
    mat_lib.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided', False)

    if uv_axes is not None:
        uv_node = _build_plane_uv_nodes(mat, mat_lib, uv_axes, x_base=-420, y_base=-60)
    else:
        uv_node = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionTextureCoordinate, -520, -60
        )
        uv_node.set_editor_property('coordinate_index', 0)
        uv_node.set_editor_property('u_tiling', float(u_tiling))
        uv_node.set_editor_property('v_tiling', float(v_tiling))

    tex_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionTextureSample, -260, -80
    )
    tex_node.set_editor_property('texture', texture_asset)
    tex_node.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    _connect(mat_lib, uv_node, '', tex_node, 'UVs')
    _connect_prop(mat_lib, tex_node, '', unreal.MaterialProperty.MP_BASE_COLOR)

    if emissive_scale > 0.0:
        em_scale = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionConstant, -260, 140
        )
        em_scale.set_editor_property('r', float(emissive_scale))
        em_mul = mat_lib.create_material_expression(
            mat, unreal.MaterialExpressionMultiply, -40, 100
        )
        _connect(mat_lib, tex_node, '', em_mul, 'A')
        _connect(mat_lib, em_scale, '', em_mul, 'B')
        _connect_prop(mat_lib, em_mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    rough_node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -260, 260
    )
    rough_node.set_editor_property('r', float(roughness))
    _connect_prop(mat_lib, rough_node, '', unreal.MaterialProperty.MP_ROUGHNESS)

    mat_lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def _build_materials(textures):
    uv_axes = _detect_plane_uv_axes()
    paper_masked = _create_paper_masked_material(uv_axes)

    paper_tints = {
        'MI_Paper_Player': (1.0, 1.0, 1.0, 1.0),
        'MI_Paper_Captain': (1.0, 1.0, 1.0, 1.0),
        'MI_Paper_Civilian': (1.0, 1.0, 1.0, 1.0),
        'MI_Paper_Extinguisher': (1.0, 1.0, 1.0, 1.0),
        'MI_Paper_Door': (1.0, 1.0, 1.0, 1.0),
        'MI_Paper_Fire': (1.0, 1.0, 1.0, 1.0),
        'MI_Paper_MatchGirl': (1.0, 1.0, 1.0, 1.0),
        'MI_Paper_AngelDog': (1.0, 1.0, 1.0, 1.0),
    }
    paper_mis = {
        name: _create_paper_material_instance(name, paper_masked, rgba)
        for name, rgba in paper_tints.items()
    }

    room_mats = {
        'M_Wall': _create_textured_material(
            'M_Wall', textures['T_Wall_VintageGreen'], u_tiling=2.0, v_tiling=2.0, roughness=0.88
        ),
        'M_Floor': _create_textured_material(
            'M_Floor', textures['T_Floor_WarmWoven'], u_tiling=3.0, v_tiling=3.0, roughness=0.85
        ),
        'M_Rug': _create_textured_material(
            'M_Rug', textures['T_Rug_LivingRoom'], u_tiling=1.0, v_tiling=1.0, roughness=0.90
        ),
        'M_NightWindow': _create_textured_material(
            'M_NightWindow',
            textures['T_NightWindow'],
            roughness=0.35,
            emissive_scale=0.45,
            uv_axes=uv_axes,
        ),
        'M_Portrait': _create_textured_material(
            'M_Portrait',
            textures['T_Portrait_Coco'],
            roughness=0.82,
            uv_axes=uv_axes,
        ),
        'M_CeilingLight': _create_solid_material(
            'M_CeilingLight',
            (0.96, 0.86, 0.64),
            roughness=0.72,
            emissive_rgb=(0.90, 0.72, 0.38),
        ),
        'M_DoorFrame': _create_solid_material('M_DoorFrame', (0.24, 0.11, 0.045), roughness=0.72),
        'M_Ceiling': _create_solid_material('M_Ceiling', (0.60, 0.56, 0.48), roughness=0.90),
        'M_Ground': _create_textured_material(
            'M_Ground', textures['T_Porch_Wood'], u_tiling=3.0, v_tiling=3.0, roughness=0.86
        ),
        'M_Porch': _create_textured_material(
            'M_Porch', textures['T_Porch_Wood'], u_tiling=3.0, v_tiling=3.0, roughness=0.86
        ),
        'M_Entry': _create_solid_material('M_Entry', (0.24, 0.11, 0.045), roughness=0.72),
        # Cool slate-taupe #6E7887 in linear RGB (~0.156, 0.188, 0.242)
        'M_Partition': _create_solid_material('M_Partition', (0.156, 0.188, 0.242), roughness=0.85),
        'M_Fire': _create_solid_material('M_Fire', (0.98, 0.22, 0.03), roughness=0.65),
        'M_SafeZone': _create_textured_material(
            'M_SafeZone', textures['T_SafeZone_Mat'], u_tiling=1.0, v_tiling=1.0, roughness=0.88
        ),
    }
    return paper_masked, paper_mis, room_mats


def _create_data_assets(textures, sounds):
    persona_class = unreal.load_class(None, '/Script/DirectorCutJam.DCPersonaDefinition')
    config_class = unreal.load_class(None, '/Script/DirectorCutJam.DCLevelConfigData')

    persona_factory = unreal.DataAssetFactory()
    persona_factory.set_editor_property('data_asset_class', persona_class)
    persona_asset = _load_or_create_asset(
        'DA_FirefighterPersona', DATA_DIR, persona_class, persona_factory
    )
    persona_asset.set_editor_property('page_header_title', 'PERSONA')
    persona_asset.set_editor_property('level_badge_title', 'Fire Call')
    persona_asset.set_editor_property('character_role_title', 'FIREFIGHTER')
    if hasattr(unreal, 'DCPersonaDefinition') and hasattr(
        unreal.DCPersonaDefinition, 'make_default_firefighter_graybox_blocks'
    ):
        blocks = unreal.DCPersonaDefinition.make_default_firefighter_graybox_blocks()
        if len(blocks) != 6:
            raise RuntimeError(f'Expected 6 v3.1 Persona blocks, got {len(blocks)}')
        persona_asset.set_editor_property('initial_blocks', blocks)
    unreal.EditorAssetLibrary.save_loaded_asset(persona_asset)

    config_factory = unreal.DataAssetFactory()
    config_factory.set_editor_property('data_asset_class', config_class)
    config_asset = _load_or_create_asset(
        'DA_FirefighterLevelConfig', DATA_DIR, config_class, config_factory
    )
    config_asset.set_editor_property('scene_title', 'Fire Call')
    config_asset.set_editor_property('level_display_name', 'Fire Call')
    config_asset.set_editor_property('default_persona_asset', persona_asset)
    config_asset.set_editor_property('seconds_to_injure_civilian', 9.5)
    config_asset.set_editor_property('seconds_to_kill_civilian', 19.5)
    config_asset.set_editor_property('seconds_to_engulf_room', 20.5)
    config_asset.set_editor_property('extinguish_action_seconds', 1.6)
    config_asset.set_editor_property('engulf_delay_per_spot_seconds', 3.5)
    config_asset.set_editor_property('initial_star_containers', 3)
    config_asset.set_editor_property('star_popup_display_seconds', 2.0)
    config_asset.set_editor_property('player_walk_speed', 320.0)
    config_asset.set_editor_property('default_interaction_radius', 175.0)
    config_asset.set_editor_property('escort_follow_distance', 90.0)
    config_asset.set_editor_property('escort_move_speed', 295.0)
    config_asset.set_editor_property('weak_escort_move_speed', 130.0)
    config_asset.set_editor_property('thought_bubble_duration_seconds', 3.8)
    config_asset.set_editor_property('camera_fov', 60.0)
    config_asset.set_editor_property('camera_world_location', unreal.Vector(-320.0, -180.0, 275.0))
    config_asset.set_editor_property(
        'camera_world_rotation', unreal.Rotator(pitch=-15.5, yaw=0.0, roll=0.0)
    )
    _set_first_valid_editor_property(
        config_asset, ['enable_smooth_pan_follow', 'b_enable_smooth_pan_follow'], True
    )
    config_asset.set_editor_property('camera_pan_follow_offset_xy', unreal.Vector2D(-740.0, 110.0))
    config_asset.set_editor_property('camera_world_bounds_min_xy', unreal.Vector2D(-350.0, -210.0))
    config_asset.set_editor_property('camera_world_bounds_max_xy', unreal.Vector2D(-190.0, 75.0))
    config_asset.set_editor_property('camera_pan_interp_speed', 4.5)
    config_asset.set_editor_property('active_fire_spot_count', 3)
    _set_first_valid_editor_property(
        config_asset, ['enable_optional_daily_block_b', 'b_enable_optional_daily_block_b'], True
    )
    config_asset.set_editor_property(
        'active_take_persona_view_policy',
        _find_enum_value(unreal.DCActiveTakePersonaViewPolicy, 'AllowTabReadOnlyPeek'),
    )
    config_asset.set_editor_property(
        'protagonist_visual_mode',
        _find_enum_value(unreal.DCCharacterVisualMode, 'Paper2D'),
    )
    config_asset.set_editor_property(
        'fire_visual_mode',
        _find_enum_value(unreal.DCFireVisualMode, 'Paper2DPlaceholder'),
    )

    audio_map = {
        'paper_open_sound': 'SFX_PaperOpen',
        'marker_swipe_sound': 'SFX_MarkerSwipe',
        'action_start_sound': 'SFX_ActionPop',
        'thought_bubble_sound': 'SFX_ThoughtBubble',
        'door_open_sound': 'SFX_DoorOpen',
        'pickup_tool_sound': 'SFX_PickupTool',
        'water_spray_sound': 'SFX_WaterSpray',
        'fire_extinguished_sound': 'SFX_FireOut',
        'star_earned_sound': 'SFX_StarEarn',
        'star_burnt_sound': 'SFX_StarBurn',
        'footstep_sound': 'SFX_Footstep',
        'stage_bgm_sound': 'BGM_StageLoop',
    }
    for prop_name, sound_key in audio_map.items():
        config_asset.set_editor_property(prop_name, sounds[sound_key])

    config_asset.set_editor_property('player_default_paper_texture', textures['T_Firefighter_Idle_0'])
    config_asset.set_editor_property(
        'player_idle_cycle_textures',
        [textures['T_Firefighter_Idle_0'], textures['T_Firefighter_Idle_1']],
    )
    config_asset.set_editor_property(
        'player_walk_cycle_textures',
        [
            textures['T_Firefighter_Walk_0'],
            textures['T_Firefighter_Walk_1'],
            textures['T_Firefighter_Walk_2'],
            textures['T_Firefighter_Walk_3'],
        ],
    )
    config_asset.set_editor_property('player_scared_texture', textures['T_Firefighter_Scared'])
    config_asset.set_editor_property('player_tool_idle_texture', textures['T_Firefighter_Tool_Idle'])
    config_asset.set_editor_property(
        'player_tool_walk_cycle_textures',
        [
            textures['T_Firefighter_Tool_Walk_0'],
            textures['T_Firefighter_Tool_Walk_1'],
            textures['T_Firefighter_Tool_Walk_2'],
            textures['T_Firefighter_Tool_Walk_3'],
        ],
    )
    config_asset.set_editor_property(
        'player_spray_cycle_textures',
        [textures['T_Firefighter_Spray_0'], textures['T_Firefighter_Spray_1']],
    )
    config_asset.set_editor_property(
        'player_escort_uninjured_idle_texture', textures['T_Firefighter_EscortUninjured_Idle']
    )
    config_asset.set_editor_property(
        'player_escort_uninjured_walk_cycle_textures',
        [
            textures['T_Firefighter_EscortUninjured_Walk_0'],
            textures['T_Firefighter_EscortUninjured_Walk_1'],
            textures['T_Firefighter_EscortUninjured_Walk_2'],
            textures['T_Firefighter_EscortUninjured_Walk_3'],
        ],
    )
    config_asset.set_editor_property(
        'player_escort_injured_idle_texture', textures['T_Firefighter_EscortInjured_Idle']
    )
    config_asset.set_editor_property(
        'player_escort_injured_walk_cycle_textures',
        [
            textures['T_Firefighter_EscortInjured_Walk_0'],
            textures['T_Firefighter_EscortInjured_Walk_1'],
            textures['T_Firefighter_EscortInjured_Walk_2'],
            textures['T_Firefighter_EscortInjured_Walk_3'],
        ],
    )
    config_asset.set_editor_property('player_water_jet_overlay_texture', textures['T_WaterJet_0'])
    config_asset.set_editor_property('buddy_uninjured_texture', textures['T_Civilian_Uninjured_0'])
    config_asset.set_editor_property('buddy_injured_texture', textures['T_Civilian_Injured_0'])

    unreal.EditorAssetLibrary.save_loaded_asset(config_asset)
    return persona_asset, config_asset


def _spawn_actor(actors_subsys, class_path, location, rotation=unreal.Rotator(), label=None):
    actor_class = unreal.load_class(None, class_path)
    if actor_class is None:
        raise RuntimeError(f'Failed to load actor class: {class_path}')
    actor = actors_subsys.spawn_actor_from_class(actor_class, location, rotation)
    if label:
        actor.set_actor_label(label)
    return actor


def _apply_room_materials(director, room_mats):
    # Exact component-name mapping matching ADCStageDirector::ApplyGrayboxMaterials()
    exact_map = {
        'Floor': 'M_Floor',
        'Ceiling': 'M_Ceiling',
        'BackWall': 'M_Wall',
        'RightWall': 'M_Wall',
        'LeftWallFront': 'M_Wall',
        'LeftWallRear': 'M_Wall',
        'LeftWallRearHeader': 'M_Wall',
        'LeftWallRearSill': 'M_DoorFrame',
        'DoorHeader': 'M_Wall',
        'PorchFloor': 'M_Porch',
        'FrontFloorApron': 'M_Ground',
        'ExteriorBackWall': 'M_Wall',
        'ExteriorLeftWall': 'M_Wall',
        'DoorFrameFrontJamb': 'M_DoorFrame',
        'DoorFrameRearJamb': 'M_DoorFrame',
        'DoorFrameLintel': 'M_DoorFrame',
        'BaseboardBack': 'M_DoorFrame',
        'BaseboardRight': 'M_DoorFrame',
        'BaseboardLeftFront': 'M_DoorFrame',
        'BaseboardLeftRear': 'M_DoorFrame',
        'CeilingFanRod': 'M_DoorFrame',
        'CeilingFanHub': 'M_DoorFrame',
        'CeilingFanBlade1': 'M_DoorFrame',
        'CeilingFanBlade2': 'M_DoorFrame',
        'CeilingFanBlade3': 'M_DoorFrame',
        'CeilingFanBlade4': 'M_DoorFrame',
        'BackWindowFrame': 'M_DoorFrame',
        'BackPortraitFrame': 'M_DoorFrame',
        'PartitionB1TrimTop': 'M_DoorFrame',
        'PartitionB2TrimTop': 'M_DoorFrame',
        'PartitionB2CabinetFront': 'M_DoorFrame',
        'PartitionB1': 'M_Partition',
        'PartitionB2': 'M_Partition',
        'AreaRug': 'M_Rug',
        'PartitionB1Cushion1': 'M_Rug',
        'PartitionB1Cushion2': 'M_Rug',
        'PartitionB1Cushion3': 'M_Rug',
        'BackWindowPane': 'M_NightWindow',
        'BackPortraitArt': 'M_Portrait',
        'CeilingLightShade': 'M_CeilingLight',
    }
    for comp in director.get_components_by_class(unreal.StaticMeshComponent):
        mat_key = exact_map.get(comp.get_name())
        if mat_key and mat_key in room_mats:
            comp.set_material(0, room_mats[mat_key])


def _make_tex_seq(idle_frames, move_frames=None):
    seq = unreal.DCStateTextureSequence()
    seq.set_editor_property('idle_frames', list(idle_frames))
    seq.set_editor_property(
        'move_frames', list(move_frames) if move_frames is not None else list(idle_frames)
    )
    return seq


def _configure_paper_visual(
    actor,
    material_instance,
    default_texture=None,
    idle_cycle=None,
    state_textures=None,
    state_sequences=None,
):
    paper_comp = actor.get_component_by_class(unreal.DCPaperVisualComponent)
    if not paper_comp:
        return
    if material_instance is not None:
        paper_comp.set_editor_property('paper_material_override', material_instance)
    if default_texture is not None:
        paper_comp.set_editor_property('default_paper_texture', default_texture)
    if idle_cycle is not None:
        paper_comp.set_editor_property('idle_cycle_textures', list(idle_cycle))
    if state_textures is not None:
        paper_comp.set_editor_property('state_textures', dict(state_textures))
    if state_sequences is not None:
        paper_comp.set_editor_property('state_texture_sequences', dict(state_sequences))
    paper_comp.refresh_visual_appearance()


def _rebuild_firefighter_map(level_config, paper_mis, room_mats, textures):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None:
        raise RuntimeError('Failed to create blank world for map rebuild.')
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    director = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCStageDirector',
        unreal.Vector(0.0, 0.0, 0.0),
        label='DC_StageDirector',
    )
    director.set_editor_property('level_config', level_config)
    director.set_editor_property('player_spawn_location', unreal.Vector(500.0, -345.0, 92.0))
    _apply_room_materials(director, room_mats)

    stage_cam = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCFixedStageCamera',
        unreal.Vector(-320.0, -180.0, 275.0),
        unreal.Rotator(pitch=-15.5, yaw=0.0, roll=0.0),
        label='DC_FixedStageCamera',
    )
    stage_cam.apply_config(level_config)

    captain = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCCaptainActor',
        unreal.Vector(370.0, -375.0, 0.0),
        label='DC_CaptainActor',
    )
    _configure_paper_visual(
        captain,
        paper_mis['MI_Paper_Captain'],
        default_texture=textures['T_Captain_Block_0'],
        state_sequences={
            'BlockEntranceArmsSpread': _make_tex_seq(
                [textures['T_Captain_Block_0'], textures['T_Captain_Block_1']]
            )
        },
    )

    doorway = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCDoorwayInteractable',
        unreal.Vector(350.0, -300.0, 0.0),
        label='DC_DoorwayInteractable',
    )
    _configure_paper_visual(
        doorway,
        paper_mis['MI_Paper_Door'],
        default_texture=textures['T_Door_Closed'],
        state_textures={'Unlocked': textures['T_Door_Open']},
    )

    extinguisher = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCExtinguisherPickup',
        unreal.Vector(320.0, 270.0, 0.0),
        label='DC_ExtinguisherPickup',
    )
    _configure_paper_visual(
        extinguisher,
        paper_mis['MI_Paper_Extinguisher'],
        default_texture=textures['T_Extinguisher'],
    )

    fire_stage1 = [
        textures['T_Fire_Stage1_0'],
        textures['T_Fire_Stage1_1'],
        textures['T_Fire_Stage1_2'],
    ]
    fire_stage2 = [
        textures['T_Fire_Stage2_0'],
        textures['T_Fire_Stage2_1'],
        textures['T_Fire_Stage2_2'],
    ]
    fire_stage3 = [
        textures['T_Fire_Stage3_0'],
        textures['T_Fire_Stage3_1'],
        textures['T_Fire_Stage3_2'],
    ]
    fire_stage4 = [
        textures['T_Fire_Stage4_0'],
        textures['T_Fire_Stage4_1'],
        textures['T_Fire_Stage4_2'],
    ]
    fire_state_seqs = {
        'InjuredCivilian': _make_tex_seq(fire_stage2),
        'KilledCivilian': _make_tex_seq(fire_stage3),
        'RoomEngulfed': _make_tex_seq(fire_stage4),
        'Spraying': _make_tex_seq(fire_stage1),
    }

    fire_spots = [
        ('DC_FireHazard_F1', 0, unreal.Vector(760.0, -220.0, 0.0)),
        ('DC_FireHazard_F2', 1, unreal.Vector(660.0, 35.0, 0.0)),
        ('DC_FireHazard_F3', 2, unreal.Vector(880.0, 160.0, 0.0)),
    ]
    for fire_label, spot_index, fire_loc in fire_spots:
        fire = _spawn_actor(
            actors,
            '/Script/DirectorCutJam.DCFireHazardActor',
            fire_loc,
            label=fire_label,
        )
        fire.set_editor_property('fire_spot_index', spot_index)
        _configure_paper_visual(
            fire,
            paper_mis['MI_Paper_Fire'],
            default_texture=textures['T_Fire_Stage1_0'],
            idle_cycle=fire_stage1,
            state_sequences=fire_state_seqs,
        )
        for comp in fire.get_components_by_class(unreal.StaticMeshComponent):
            comp_name = comp.get_name()
            if 'FireCore' in comp_name or 'FireOuter' in comp_name:
                comp.set_material(0, room_mats['M_Fire'])

    civilian = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCCivilianActor',
        unreal.Vector(885.0, -95.0, 0.0),
        label='DC_CivilianActor',
    )
    civ_uninjured = [textures['T_Civilian_Uninjured_0'], textures['T_Civilian_Uninjured_1']]
    civ_injured = [textures['T_Civilian_Injured_0'], textures['T_Civilian_Injured_1']]
    civ_rescued = [textures['T_Civilian_Rescued_0'], textures['T_Civilian_Rescued_1']]
    _configure_paper_visual(
        civilian,
        paper_mis['MI_Paper_Civilian'],
        default_texture=textures['T_Civilian_Uninjured_0'],
        idle_cycle=civ_uninjured,
        state_textures={'Dead': textures['T_Civilian_Dead']},
        state_sequences={
            'Injured': _make_tex_seq(civ_injured),
            'Rescued': _make_tex_seq(civ_rescued),
        },
    )

    match_girl = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCMatchGirlActor',
        unreal.Vector(225.0, 335.0, 0.0),
        label='DC_MatchGirlActor',
    )
    _configure_paper_visual(
        match_girl,
        paper_mis['MI_Paper_MatchGirl'],
        default_texture=textures['T_MatchGirl_Idle_0'],
        idle_cycle=[textures['T_MatchGirl_Idle_0'], textures['T_MatchGirl_Idle_1']],
    )

    angel_dog = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCAngelDogActor',
        unreal.Vector(415.0, -460.0, 0.0),
        label='DC_AngelDogActor',
    )
    _configure_paper_visual(
        angel_dog,
        paper_mis['MI_Paper_AngelDog'],
        default_texture=textures['T_AngelDog_Idle_0'],
        idle_cycle=[textures['T_AngelDog_Idle_0'], textures['T_AngelDog_Idle_1']],
        state_sequences={
            'Happy': _make_tex_seq(
                [textures['T_AngelDog_Happy_0'], textures['T_AngelDog_Happy_1']]
            )
        },
    )

    safe_zone = _spawn_actor(
        actors,
        '/Script/DirectorCutJam.DCSafeZoneVolume',
        unreal.Vector(420.0, -385.0, 0.0),
        label='DC_SafeZoneVolume',
    )
    safe_zone.set_editor_property('civilian_park_offset', unreal.Vector(-100.0, -15.0, 0.0))
    for comp in safe_zone.get_components_by_class(unreal.StaticMeshComponent):
        comp.set_material(0, room_mats['M_SafeZone'])

    player_start = actors.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(500.0, -345.0, 92.0)
    )
    player_start.set_actor_label('Firefighter_Start')

    dir_light = actors.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 1000.0),
        unreal.Rotator(pitch=-55.0, yaw=-30.0, roll=0.0),
    )
    dir_light.light_component.set_editor_property('intensity', 3.5)
    dir_light.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)

    sky_light = actors.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0.0, 0.0, 900.0)
    )
    sky_light.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    sky_light.light_component.set_editor_property('intensity', 1.0)
    sky_light.light_component.set_editor_property(
        'source_type', unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP
    )
    cube = unreal.load_asset('/Engine/EngineResources/DefaultTextureCube.DefaultTextureCube')
    if cube:
        sky_light.light_component.set_editor_property('cubemap', cube)

    game_mode = unreal.load_class(None, '/Script/DirectorCutJam.DCGameMode')
    editor_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world() or world
    editor_world.get_world_settings().set_editor_property('default_game_mode', game_mode)

    if not unreal.EditorLoadingAndSavingUtils.save_map(editor_world, MAP_PATH):
        raise RuntimeError(f'Failed to save rebuilt map: {MAP_PATH}')
    unreal.EditorAssetLibrary.save_directory('/Game')
    if not levels.load_level(MAP_PATH):
        raise RuntimeError(f'Failed to reload saved map: {MAP_PATH}')
    unreal.log('DC_MAP_CREATED ' + MAP_PATH)


def main():
    cmd_line = unreal.SystemLibrary.get_command_line()
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH) and '-DCRebuildMap' not in cmd_line:
        unreal.log('DC map already exists; leaving authored map untouched.')
        return

    textures, sounds = _import_assets()
    _, paper_mis, room_mats = _build_materials(textures)
    _, level_config = _create_data_assets(textures, sounds)
    _rebuild_firefighter_map(level_config, paper_mis, room_mats, textures)


if __name__ == '__main__':
    main()
