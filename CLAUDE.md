# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AL_MovementLab is a UE5 (5.6) C++ project implementing an Apex Legends-style momentum-based movement system and projectile-based weapon system. It extends `UCharacterMovementComponent` with sprint, sliding, crouch walking, wall running, double jump, and a momentum accumulation mechanic.

## Build Commands

```bash
# Generate Visual Studio project files (run from project root)
# Right-click AL_MovementLab.uproject -> "Generate Visual Studio project files"

# Build from Visual Studio
# Open AL_MovementLab.sln, select "Development Editor" configuration, press F7

# Hot Reload (while editor is running)
# In Visual Studio: Build the project, or in UE Editor: Ctrl+Alt+F11

# Open in Unreal Editor
# Double-click AL_MovementLab.uproject
```

## Rebuilding After Lost Files / Clean Build

If Binaries/Intermediate are missing or corrupted:

1. Delete these folders if they exist:
   - `Binaries/`
   - `Intermediate/`
   - `Saved/`
   - `.vs/`
   - `*.sln`

2. Right-click `AL_MovementLab.uproject` → "Generate Visual Studio project files"

3. Open the generated `.sln` in Visual Studio

4. Build → Build Solution (F7)

5. Launch via Visual Studio (F5) or double-click the `.uproject`

## Architecture

### Core Classes

**AALCharacter** (`Source/AL_MovementLab/ALCharacter.h/.cpp`)
- First-person character with camera-only crouch (no capsule resize)
- Handles input binding (legacy input system) and camera height interpolation
- Routes movement input to movement component
- Routes weapon input to weapon actor
- Spawns and manages weapon attachment
- Key members: `FirstPersonCamera`, `CurrentWeapon`, `bIsCrouching`, `TargetCameraHeight`

**UALCharacterMovementComponent** (`Source/AL_MovementLab/ALCharacterMovementComponent.h/.cpp`)
- Extends `UCharacterMovementComponent`
- Implements momentum system, sprint, ground slide, air slide, crouch walk, wall running, double jump, air strafe
- Public API: `SetIsSprinting()`, `StartCrouch()`, `StopCrouch()`, `TryWallRun()`, `WallJump()`, `DoubleJump()`
- Slide functions are private; routing logic in `StartCrouch()` determines slide vs crouch walk
- `ProcessLanded()` override applies deferred air slide impulse

**AALWeapon** (`Source/AL_MovementLab/ALWeapon.h/.cpp`)
- Projectile-based weapon with hipfire/ADS modes
- Visual recoil system (positional + rotational kick with recovery)
- Stow/draw animation system
- Spawns `AALProjectile` actors on fire
- Key members: `WeaponMesh`, `ProjectileClass`, `bIsADS`, `bIsStowed`
- Public API: `Fire()`, `StartFire()`, `StopFire()`, `StartADS()`, `StopADS()`, `Stow()`, `Draw()`

**AALProjectile** (`Source/AL_MovementLab/ALProjectile.h/.cpp`)
- Physics-based projectile with travel time and gravity
- Uses `UProjectileMovementComponent` for movement
- Hit detection and damage application
- Key members: `ProjectileSpeed`, `ProjectileGravityScale`, `Damage`

### Movement State Flow

```
StartCrouch() called
    |
    v
[Check conditions]
    |
    +-- Airborne + Speed >= 650 + Momentum >= 30 --> StartAirSlide()
    |
    +-- Grounded + Speed >= 650 + Momentum >= 15 --> StartGroundSlide()
    |
    +-- Otherwise --> StartCrouchWalk()
```

Air slide impulse is deferred until `ProcessLanded()` fires.

### Weapon State Flow

```
[DRAWN - Hipfire]
    |
    +-- Right Mouse Hold --> [ADS] (weapon centers, no spread)
    |
    +-- Press 3 --> [STOWED] (weapon animates down, can't fire)

[STOWED]
    |
    +-- Press 1 --> [DRAWN - Hipfire]
```

### Key Design Decisions

- Engine crouch disabled (`bCanCrouch = false`) to prevent capsule resize jitter
- Camera height interpolation via `FMath::FInterpTo` in character's Tick
- Momentum decays faster during crouch walk, slower while airborne
- Ground slide: 50% impulse, 15 momentum cost; Air slide: 120% impulse, 30 momentum cost
- Weapon attached to camera, position managed by weapon's Tick
- Hipfire spawns projectiles from barrel, aims toward crosshair (line trace)
- ADS spawns projectiles from camera center
- Visual recoil is additive and recovers smoothly

## Input Bindings (Legacy System)

The project uses UE4-style axis/action mappings. Configure in Project Settings > Input:

**Movement:**
- Axes: `MoveForward`, `MoveRight`, `Turn`, `LookUp`
- Actions: `Sprint`, `Jump`, `Crouch`

**Weapon:**
- Actions: `Fire` (Left Mouse), `ADS` (Right Mouse), `DrawWeapon` (1), `StowWeapon` (3)

## Debug Display

Set `bDrawMomentumDebug = true` on movement component to show on-screen debug info:
- Line 1 (Cyan): Speed and momentum values
- Line 2 (Green): Ground/air state, sprint state, movement mode
- Line 3 (Color-coded): Slide eligibility (Green = air ready, Yellow = ground ready, Orange = no slide)

## Blueprint Setup

**BP_ALCharacter:**
- Set `WeaponClass` to `BP_ALWeapon`

**BP_ALWeapon:**
- Set `WeaponMesh` skeletal mesh (e.g., SK_Rifle)
- Set `ProjectileClass` to `BP_ALProjectile`
- Adjust position/recoil parameters as needed

**BP_ALProjectile:**
- Add a visible static mesh to `MeshComponent` (e.g., small sphere)
- Adjust `ProjectileSpeed`, `ProjectileGravityScale`, `Damage` as needed
