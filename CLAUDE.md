# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AL_MovementLab is a UE5 (5.6) C++ project implementing an Apex Legends-style momentum-based movement system. It extends `UCharacterMovementComponent` with sprint, sliding, crouch walking, and a momentum accumulation mechanic.

## Build Commands

```bash
# Generate Visual Studio project files (run from project root)
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "%CD%\AL_MovementLab.uproject" -Game

# Build from command line (Development Editor)
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" AL_MovementLabEditor Win64 Development "%CD%\AL_MovementLab.uproject"

# Open in Unreal Editor
"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\AL_MovementLab.uproject"
```

Alternatively, open the `.sln` file in Visual Studio and build via IDE (F7 or Build > Build Solution).

## Architecture

### Core Classes

**AALCharacter** (`Source/AL_MovementLab/ALCharacter.h/.cpp`)
- First-person character with camera-only crouch (no capsule resize)
- Handles input binding (legacy input system) and camera height interpolation
- Routes crouch/sprint input to movement component
- Key members: `FirstPersonCamera`, `bIsCrouching`, `TargetCameraHeight`

**UALCharacterMovementComponent** (`Source/AL_MovementLab/ALCharacterMovementComponent.h/.cpp`)
- Extends `UCharacterMovementComponent`
- Implements momentum system, sprint, ground slide, air slide, crouch walk
- Public API: `SetIsSprinting()`, `StartCrouch()`, `StopCrouch()`, `IsSliding()`, `IsCrouchWalking()`
- Slide functions are private; routing logic in `StartCrouch()` determines slide vs crouch walk
- `ProcessLanded()` override applies deferred air slide impulse

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

### Key Design Decisions

- Engine crouch disabled (`bCanCrouch = false`) to prevent capsule resize jitter
- Camera height interpolation via `FMath::FInterpTo` in character's Tick
- Momentum decays faster during crouch walk, slower while airborne
- Ground slide: 50% impulse, 15 momentum cost; Air slide: 120% impulse, 30 momentum cost

## Input Bindings (Legacy System)

The project uses UE4-style axis/action mappings. These must be configured in Project Settings > Input:
- Axes: `MoveForward`, `MoveRight`, `Turn`, `LookUp`
- Actions: `Sprint`, `Jump`, `Crouch`

## Debug Display

Set `bDrawMomentumDebug = true` on movement component to show on-screen debug info:
- Line 1 (Cyan): Speed and momentum values
- Line 2 (Green): Ground/air state, sprint state, movement mode
- Line 3 (Color-coded): Slide eligibility (Green = air ready, Yellow = ground ready, Orange = no slide)
