"""Read-only dump of KINEMATION pack class internals (safe with editor open)."""
import unreal

_ar = unreal.AssetRegistryHelpers.get_asset_registry()
_ar.wait_for_completion()
_ar.scan_paths_synchronous(['/FPSAnimationPack', '/Game/Player'], force_rescan=True)


def dump_class(label, path):
    cls = unreal.load_object(None, path)
    if not cls:
        unreal.log('[introspect] %s: FAILED to load %s' % (label, path))
        return None
    unreal.log('[introspect] === %s (%s) ===' % (label, cls.get_name()))
    cdo = unreal.get_default_object(cls)
    # Walk the class hierarchy via python reflection on the CDO
    for prop in dir(cdo):
        if prop.startswith('_'):
            continue
        try:
            val = cdo.get_editor_property(prop)
        except Exception:
            continue
        cls_name = type(val).__name__
        if cls_name in ('NoneType',):
            unreal.log('[introspect]   %s = None' % prop)
        elif cls_name in ('bool', 'int', 'float', 'str', 'Name', 'Text'):
            unreal.log('[introspect]   %s = %r (%s)' % (prop, str(val), cls_name))
        elif cls_name == 'Array':
            unreal.log('[introspect]   %s = Array[%d]' % (prop, len(val)))
            for i, item in enumerate(list(val)[:8]):
                unreal.log('[introspect]     [%d] %s' % (i, item))
        else:
            unreal.log('[introspect]   %s = %s (%s)' % (prop, val, cls_name))
    return cls


# Pack component + weapon classes
dump_class('WeaponManager', '/FPSAnimationPack/Blueprints/Character/WeaponManager.WeaponManager_C')
dump_class('ViewmodelController', '/FPSAnimationPack/Blueprints/Character/ViewmodelController.ViewmodelController_C')
dump_class('BP_WeaponBase', '/FPSAnimationPack/Blueprints/Weapon/BP_WeaponBase.BP_WeaponBase_C')

# Data assets / enums
cs = unreal.EditorAssetLibrary.load_asset('/FPSAnimationPack/Blueprints/Character/CS_Mannequin')
if cs:
    unreal.log('[introspect] === CS_Mannequin (%s) ===' % cs.get_class().get_name())
    for prop in dir(cs):
        if prop.startswith('_'):
            continue
        try:
            val = cs.get_editor_property(prop)
        except Exception:
            continue
        if type(val).__name__ == 'Array':
            unreal.log('[introspect]   %s = Array[%d]: %s' % (prop, len(val), [str(v) for v in list(val)[:8]]))
        else:
            unreal.log('[introspect]   %s = %s' % (prop, val))

da = unreal.EditorAssetLibrary.load_asset('/FPSAnimationPack/Blueprints/Weapon/Settings/DA_MX16A4')
if da:
    unreal.log('[introspect] === DA_MX16A4 (%s) ===' % da.get_class().get_name())
    for prop in dir(da):
        if prop.startswith('_'):
            continue
        try:
            val = da.get_editor_property(prop)
        except Exception:
            continue
        unreal.log('[introspect]   %s = %s' % (prop, val))

enum = unreal.load_object(None, '/FPSAnimationPack/Blueprints/Character/E_FireMode.E_FireMode')
unreal.log('[introspect] E_FireMode object: %s' % enum)

# SCS component variable names on our duplicated pawn (instances appear as
# properties on the generated class CDO)
dump_class('BP_AZViewmodel', '/Game/Player/BP_AZViewmodel.BP_AZViewmodel_C')
