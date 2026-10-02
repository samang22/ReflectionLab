# Player component migration

The native classes now own the previous Character configuration. The captured
BP_RLPlayerCharacter values are preserved in MigratePlayerComponents.py.
DA_PlayerStats and all animation assets remain unchanged.

1. Build ReflectionLabEditor with the editor closed (a full build, not Live Coding).
2. Open the editor, then choose Tools > Execute Python Script and select
   Scripts/Editor/MigratePlayerComponents.py.
3. Check BP_RLPlayerCharacter for compile errors before gameplay testing.
   The script saves only this blueprint after comparing the migrated values.
4. Close and reopen the editor and run verification with the command below.
   This fresh process checks persisted settings, not just the in-memory CDO.

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Report\Projects\ReflectionLab\ReflectionLab.uproject" -run=pythonscript -script="C:\Report\Projects\ReflectionLab\Scripts\Editor\MigratePlayerComponents.py" -VerifyPlayerComponents -unattended -nullrhi -nosound
```

Success marker: PLAYER_COMPONENT_DEFAULTS_VERIFIED.

Do not rerun migration after intentionally editing component settings: it restores
the captured pre-refactor configuration. SnapshotPlayerComponents.py uses the OLD
DLL/property layout and must not be rerun with the new module.

Gameplay checks after building: normal/perfect/close parry, multi-reflection healing
once, level advancement on the milestone swing, overdrive consumption, mirrored
montage alternation, failed-parry cooldown, hit recovery/death interrupting actions,
reward stacking/reset, indicator/aura/sounds/hit-stop restoration, root-motion roll.

Existing player automation tests moved to Private/Tests. They were NOT executed as
part of this refactoring, per the user's request.

