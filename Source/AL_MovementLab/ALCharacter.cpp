// ALCharacter.cpp

#include "ALCharacter.h"
#include "ALCharacterMovementComponent.h"
#include "ALWeapon.h"

#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "UObject/UnrealType.h"

AALCharacter::AALCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(
		ObjectInitializer.SetDefaultSubobjectClass<UALCharacterMovementComponent>(
			ACharacter::CharacterMovementComponentName
		)
	)
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = true;

	if (UALCharacterMovementComponent* MoveComp = Cast<UALCharacterMovementComponent>(GetCharacterMovement()))
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->GetNavAgentPropertiesRef().bCanCrouch = false;
	}

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(RootComponent);
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, StandingCameraHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

	BaseEyeZ = StandingCameraHeight;
	CurrentEyeOffset = 0.f;
	TargetEyeOffset = 0.f;
	TargetCameraRoll = 0.f;
	CurrentCameraRoll = 0.f;
	bIsCrouching = false;
	bWantsToMoveForward = false;
}

void AALCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Capture the resting eye height; the crouch dip is applied as an offset
	// from this so it works for any EyeHeightComponent, not just the camera.
	TargetEyeOffset = 0.f;
	CurrentEyeOffset = 0.f;
	if (USceneComponent* Eye = GetEyeComponent())
	{
		BaseEyeZ = (Eye == FirstPersonCamera) ? StandingCameraHeight : Eye->GetRelativeLocation().Z;
		FVector EyeLoc = Eye->GetRelativeLocation();
		EyeLoc.Z = BaseEyeZ;
		Eye->SetRelativeLocation(EyeLoc);
	}

	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(DefaultFOV);
	}

	if (ViewmodelContextsToRemove.Num() > 0 || ViewmodelContextsToAdd.Num() > 0)
	{
		// Deferred a tick: the blueprint BeginPlay graph (which adds the
		// context we may want gone) runs after this C++ body.
		GetWorldTimerManager().SetTimerForNextTick(this, &AALCharacter::ApplyViewmodelInputContexts);
	}

	// Spawn weapon
	SpawnWeapon();
}

void AALCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Try to wall run if holding forward and airborne
	if (bWantsToMoveForward)
	{
		if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
		{
			ALMove->TryWallRun();
		}
	}

	// Eye height (crouch/slide dip) interpolation
	if (USceneComponent* Eye = GetEyeComponent())
	{
		if (!FMath::IsNearlyEqual(CurrentEyeOffset, TargetEyeOffset, 0.1f))
		{
			CurrentEyeOffset = FMath::FInterpTo(CurrentEyeOffset, TargetEyeOffset, DeltaTime, CrouchCameraInterpSpeed);
			FVector EyeLoc = Eye->GetRelativeLocation();
			EyeLoc.Z = BaseEyeZ + CurrentEyeOffset;
			Eye->SetRelativeLocation(EyeLoc);
		}
	}

	// ADS FOV zoom
	if (FirstPersonCamera)
	{
		const float TargetFOV = (CurrentWeapon && CurrentWeapon->IsADS()) ? ADSFOV : DefaultFOV;
		if (!FMath::IsNearlyEqual(FirstPersonCamera->FieldOfView, TargetFOV, 0.01f))
		{
			FirstPersonCamera->SetFieldOfView(
				FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, ADSFOVInterpSpeed));
		}
	}

	// Camera tilt for wall running
	UpdateCameraTilt(DeltaTime);

	// Viewmodel state feed, fire-mode enforcement, on-screen readout
	UpdateViewmodelMovementState();
	UpdateViewmodelDebug();
}

void AALCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AALCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AALCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AALCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AALCharacter::LookUp);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AALCharacter::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AALCharacter::StopSprint);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AALCharacter::OnJumpPressed);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AALCharacter::OnJumpReleased);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AALCharacter::StartCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AALCharacter::StopCrouch);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AALCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AALCharacter::StopFire);

	PlayerInputComponent->BindAction("ADS", IE_Pressed, this, &AALCharacter::StartADS);
	PlayerInputComponent->BindAction("ADS", IE_Released, this, &AALCharacter::StopADS);

	PlayerInputComponent->BindAction("DrawWeapon", IE_Pressed, this, &AALCharacter::DrawWeapon);
	PlayerInputComponent->BindAction("StowWeapon", IE_Pressed, this, &AALCharacter::StowWeapon);
}

// ---- Movement ----

void AALCharacter::MoveForward(float Value)
{
	// Track if player is holding forward for wall run detection
	bWantsToMoveForward = (Value > 0.f);

	if (Controller && Value != 0.0f)
	{
		const FRotator ControlRot = Controller->GetControlRotation();
		const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AALCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.0f)
	{
		const FRotator ControlRot = Controller->GetControlRotation();
		const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

// ---- Mouse Look ----

void AALCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AALCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

// ---- Sprint ----

void AALCharacter::StartSprint()
{
	if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
	{
		ALMove->SetIsSprinting(true);
	}
}

void AALCharacter::StopSprint()
{
	if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
	{
		ALMove->SetIsSprinting(false);
	}
}

// ---- Crouch / Slide ----

void AALCharacter::StartCrouch()
{
	if (bIsCrouching)
	{
		return;
	}

	bIsCrouching = true;
	TargetEyeOffset = CrouchingCameraHeight - StandingCameraHeight;

	if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
	{
		ALMove->StartCrouch();
	}
}

void AALCharacter::StopCrouch()
{
	if (!bIsCrouching)
	{
		return;
	}

	bIsCrouching = false;
	TargetEyeOffset = 0.f;

	if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
	{
		ALMove->StopCrouch();
	}
}

UALCharacterMovementComponent* AALCharacter::GetALMovementComponent() const
{
	return Cast<UALCharacterMovementComponent>(GetCharacterMovement());
}

USceneComponent* AALCharacter::GetEyeComponent() const
{
	return EyeHeightComponent ? EyeHeightComponent.Get() : Cast<USceneComponent>(FirstPersonCamera);
}

// ---- Viewmodel Input ----

void AALCharacter::ApplyViewmodelInputContexts()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!Subsystem)
	{
		return;
	}

	for (const TSoftObjectPtr<UInputMappingContext>& Context : ViewmodelContextsToRemove)
	{
		if (UInputMappingContext* Resolved = Context.LoadSynchronous())
		{
			Subsystem->RemoveMappingContext(Resolved);
		}
	}

	for (const TSoftObjectPtr<UInputMappingContext>& Context : ViewmodelContextsToAdd)
	{
		if (UInputMappingContext* Resolved = Context.LoadSynchronous())
		{
			Subsystem->AddMappingContext(Resolved, ViewmodelContextPriority);
		}
	}
}

// ---- Jump / Wall Jump ----

void AALCharacter::OnJumpPressed()
{
	if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
	{
		// Wall jump takes priority
		if (ALMove->IsWallRunning())
		{
			ALMove->WallJump();
			return;
		}

		// Try double jump if airborne
		if (ALMove->CanDoubleJump())
		{
			ALMove->DoubleJump();
			return;
		}
	}

	Jump();
}

void AALCharacter::OnJumpReleased()
{
	StopJumping();
}

// ---- Camera Tilt ----

void AALCharacter::UpdateCameraTilt(float DeltaTime)
{
	UALCharacterMovementComponent* ALMove = GetALMovementComponent();
	if (!ALMove || !Controller)
	{
		return;
	}

	// Determine target roll based on wall run state
	if (ALMove->IsWallRunning())
	{
		float TiltAngle = ALMove->GetWallRunCameraTilt();
		// Tilt toward the wall (right wall = positive roll, left wall = negative roll)
		TargetCameraRoll = ALMove->IsWallRunningOnRightSide() ? TiltAngle : -TiltAngle;
	}
	else
	{
		TargetCameraRoll = 0.f;
	}

	// Interpolate current roll toward target
	if (!FMath::IsNearlyEqual(CurrentCameraRoll, TargetCameraRoll, 0.1f))
	{
		CurrentCameraRoll = FMath::FInterpTo(CurrentCameraRoll, TargetCameraRoll, DeltaTime, WallRunCameraTiltInterpSpeed);

		// Apply roll to controller
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			FRotator ControlRot = PC->GetControlRotation();
			ControlRot.Roll = CurrentCameraRoll;
			PC->SetControlRotation(ControlRot);
		}
	}
}

// ---- Weapon ----

void AALCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AALCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AALCharacter::StartADS()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartADS();
	}
}

void AALCharacter::StopADS()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopADS();
	}
}

void AALCharacter::DrawWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Draw();
	}
}

void AALCharacter::StowWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Stow();
	}
}

// ---- Viewmodel (FPS Animation pack) ----
// The pack's WeaponManager component and weapon actors are blueprint classes,
// so their properties are reached via reflection by name (names confirmed
// against the pack's asset name tables: ActiveWeapon, ActiveSettings, FireMode).

static UEnum* ResolveEnumProperty(const FProperty* Prop, const void* Container, int64& OutValue)
{
	const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Container);
	if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
	{
		OutValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
		return EnumProp->GetEnum();
	}
	if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
	{
		OutValue = *static_cast<const uint8*>(ValuePtr);
		return ByteProp->Enum;
	}
	return nullptr;
}

void AALCharacter::UpdateViewmodelMovementState()
{
	if (!bDriveViewmodelMovementState)
	{
		return;
	}

	UALCharacterMovementComponent* ALMove = GetALMovementComponent();
	FProperty* StateProp = GetClass()->FindPropertyByName(TEXT("MovementState"));
	if (!ALMove || !StateProp)
	{
		return;
	}

	const float Speed2D = GetVelocity().Size2D();

	// Map our state machine onto the pack's E_MovementState. Slides hold the
	// weapon steady (Idle); tac-sprint is the visual reward for momentum.
	const TCHAR* Desired = TEXT("Idle");
	if (ALMove->IsSliding())
	{
		Desired = TEXT("Idle");
	}
	else if ((ALMove->IsSprinting() || ALMove->IsWallRunning()) && Speed2D >= SprintAnimSpeedThreshold)
	{
		Desired = (ALMove->GetMomentum() >= TacSprintMomentumThreshold) ? TEXT("TacSprint") : TEXT("Sprint");
	}
	else if (Speed2D > 25.f)
	{
		Desired = TEXT("Walk");
	}

	int64 CurrentValue = 0;
	UEnum* Enum = ResolveEnumProperty(StateProp, this, CurrentValue);
	if (!Enum)
	{
		return;
	}

	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
	{
		if (Enum->GetDisplayNameTextByIndex(i).ToString().Replace(TEXT(" "), TEXT("")) != Desired)
		{
			continue;
		}
		const int64 NewValue = Enum->GetValueByIndex(i);
		if (NewValue == CurrentValue)
		{
			return;
		}
		void* ValuePtr = StateProp->ContainerPtrToValuePtr<void>(this);
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(StateProp))
		{
			EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, NewValue);
		}
		else
		{
			*static_cast<uint8*>(ValuePtr) = static_cast<uint8>(NewValue);
		}
		return;
	}
}

AActor* AALCharacter::GetActiveViewmodelWeapon() const
{
	for (UActorComponent* Comp : GetComponents())
	{
		if (!Comp || !Comp->GetClass()->GetName().StartsWith(TEXT("WeaponManager")))
		{
			continue;
		}
		if (const FObjectProperty* Prop = FindFProperty<FObjectProperty>(Comp->GetClass(), TEXT("ActiveWeapon")))
		{
			return Cast<AActor>(Prop->GetObjectPropertyValue_InContainer(Comp));
		}
	}
	return nullptr;
}

void AALCharacter::UpdateViewmodelDebug()
{
	AActor* Weapon = GetActiveViewmodelWeapon();

	// One-shot flip to full auto whenever the equipped weapon changes
	if (Weapon && bForceAutoFireMode && Weapon != LastFireModeWeapon)
	{
		if (FProperty* ModeProp = Weapon->GetClass()->FindPropertyByName(TEXT("FireMode")))
		{
			int64 CurrentValue = 0;
			if (UEnum* Enum = ResolveEnumProperty(ModeProp, Weapon, CurrentValue))
			{
				for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
				{
					if (!Enum->GetDisplayNameTextByIndex(i).ToString().Contains(TEXT("Auto")))
					{
						continue;
					}
					const int64 AutoValue = Enum->GetValueByIndex(i);
					void* ValuePtr = ModeProp->ContainerPtrToValuePtr<void>(Weapon);
					if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(ModeProp))
					{
						EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, AutoValue);
					}
					else
					{
						*static_cast<uint8*>(ValuePtr) = static_cast<uint8>(AutoValue);
					}
					break;
				}
			}
		}
		LastFireModeWeapon = Weapon;
	}

	if (!bShowViewmodelDebug || !GEngine || !IsLocallyControlled())
	{
		return;
	}

	FString WeaponName = TEXT("none");
	FString ModeName = TEXT("-");
	if (Weapon)
	{
		WeaponName = Weapon->GetClass()->GetName();
		if (const FObjectProperty* SettingsProp = FindFProperty<FObjectProperty>(Weapon->GetClass(), TEXT("ActiveSettings")))
		{
			if (UObject* Settings = SettingsProp->GetObjectPropertyValue_InContainer(Weapon))
			{
				WeaponName = Settings->GetName();
				WeaponName.RemoveFromStart(TEXT("DA_"));
			}
		}
		if (const FProperty* ModeProp = Weapon->GetClass()->FindPropertyByName(TEXT("FireMode")))
		{
			int64 ModeValue = 0;
			if (const UEnum* Enum = ResolveEnumProperty(ModeProp, Weapon, ModeValue))
			{
				ModeName = Enum->GetDisplayNameTextByValue(ModeValue).ToString();
			}
		}
	}

	const UAnimMontage* Montage = nullptr;
	if (const USkeletalMeshComponent* Arms = GetMesh())
	{
		if (UAnimInstance* Anim = Arms->GetAnimInstance())
		{
			Montage = Anim->GetCurrentActiveMontage();
		}
	}

	FString MoveState = TEXT("WALK");
	if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
	{
		if (ALMove->IsWallRunning())        { MoveState = TEXT("WALLRUN"); }
		else if (ALMove->IsSliding())       { MoveState = TEXT("SLIDE"); }
		else if (ALMove->IsCrouchWalking()) { MoveState = TEXT("CROUCH"); }
		else if (ALMove->IsFalling())       { MoveState = TEXT("AIR"); }
		else if (ALMove->IsSprinting())     { MoveState = TEXT("SPRINT"); }
	}

	const uint64 Key = (uint64)((PTRINT)this);
	GEngine->AddOnScreenDebugMessage(Key + 10, 0.f, FColor::Orange,
		FString::Printf(TEXT("Weapon: %s  |  Mode: %s"), *WeaponName, *ModeName));
	GEngine->AddOnScreenDebugMessage(Key + 11, 0.f, FColor::Yellow,
		FString::Printf(TEXT("Montage: %s"), Montage ? *Montage->GetName() : TEXT("none")));
	GEngine->AddOnScreenDebugMessage(Key + 12, 0.f, FColor::Cyan,
		FString::Printf(TEXT("Anim State: %s  |  Speed: %.0f"), *MoveState, GetVelocity().Size2D()));
}

void AALCharacter::SpawnWeapon()
{
	if (!WeaponClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	CurrentWeapon = GetWorld()->SpawnActor<AALWeapon>(WeaponClass, SpawnParams);
	if (CurrentWeapon)
	{
		// Attach weapon to camera so it follows view
		CurrentWeapon->AttachToComponent(
			FirstPersonCamera,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
		// Position is managed by weapon's Tick based on ADS state
		CurrentWeapon->SetActorRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}
}