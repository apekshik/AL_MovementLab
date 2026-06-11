import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()
ar.wait_for_completion()

roots = ar.get_sub_paths('/', recurse=False)
unreal.log('[diag] content roots: %s' % sorted(str(r) for r in roots))

ar.scan_paths_synchronous(['/FPSAnimationPack'], force_rescan=True)
assets = ar.get_assets_by_path('/FPSAnimationPack/Blueprints/Character', recursive=True)
unreal.log('[diag] after explicit scan, character assets: %s' %
           [str(a.asset_name) for a in assets])

obj = unreal.load_object(None, '/FPSAnimationPack/Blueprints/Character/BP_ViewmodelCharacter.BP_ViewmodelCharacter')
unreal.log('[diag] direct load_object: %s' % obj)
