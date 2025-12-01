// ALCharacter.cpp

#include "ALCharacter.h"
#include "ALCharacterMovementComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"

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

	TargetCameraHeight = StandingCameraHeight;
	TargetCameraRoll = 0.f;
	CurrentCameraRoll = 0.f;
	bIsCrouching = false;
	bWantsToMoveForward = false;
}

void AALCharacter::BeginPlay()
{
	Super::BeginPlay();

	TargetCameraHeight = StandingCameraHeight;
	if (FirstPersonCamera)
	{
		FVector CamLoc = FirstPersonCamera->GetRelativeLocation();
		CamLoc.Z = StandingCameraHeight;
		FirstPersonCamera->SetRelativeLocation(CamLoc);
	}
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

	// Camera height interpolation
	if (FirstPersonCamera)
	{
		FVector CamLoc = FirstPersonCamera->GetRelativeLocation();

		if (!FMath::IsNearlyEqual(CamLoc.Z, TargetCameraHeight, 0.1f))
		{
			CamLoc.Z = FMath::FInterpTo(CamLoc.Z, TargetCameraHeight, DeltaTime, CrouchCameraInterpSpeed);
			FirstPersonCamera->SetRelativeLocation(CamLoc);
		}
	}

	// Camera tilt for wall running
	UpdateCameraTilt(DeltaTime);
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
	TargetCameraHeight = CrouchingCameraHeight;

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
	TargetCameraHeight = StandingCameraHeight;

	if (UALCharacterMovementComponent* ALMove = GetALMovementComponent())
	{
		ALMove->StopCrouch();
	}
}

UALCharacterMovementComponent* AALCharacter::GetALMovementComponent() const
{
	return Cast<UALCharacterMovementComponent>(GetCharacterMovement());
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