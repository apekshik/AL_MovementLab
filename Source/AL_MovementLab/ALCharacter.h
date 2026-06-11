// ALCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALCharacter.generated.h"

class UCameraComponent;
class UALCharacterMovementComponent;
class AALWeapon;
class AALProjectile;
class UInputMappingContext;
class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;

UCLASS()
class AL_MOVEMENTLAB_API AALCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AALCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UALCharacterMovementComponent* GetALMovementComponent() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Component whose relative Z gets the crouch/slide eye dip. When unset,
	// falls back to FirstPersonCamera. For viewmodel pawns set this to the
	// arms mesh so the socketed camera follows.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	TObjectPtr<USceneComponent> EyeHeightComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float StandingCameraHeight = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float CrouchingCameraHeight = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float CrouchCameraInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|WallRun")
	float WallRunCameraTiltInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	float DefaultFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	float ADSFOV = 72.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|ADS")
	float ADSFOVInterpSpeed = 12.f;

	// Enhanced Input contexts swapped one tick after BeginPlay. Lets a child
	// blueprint replace a context its own BeginPlay graph adds (e.g. the FPS
	// Animation pack's full IMC) with a trimmed project-owned one, without
	// editing the blueprint graph.
	UPROPERTY(EditAnywhere, Category = "Input|Viewmodel")
	TArray<TSoftObjectPtr<UInputMappingContext>> ViewmodelContextsToRemove;

	UPROPERTY(EditAnywhere, Category = "Input|Viewmodel")
	TArray<TSoftObjectPtr<UInputMappingContext>> ViewmodelContextsToAdd;

	UPROPERTY(EditAnywhere, Category = "Input|Viewmodel")
	int32 ViewmodelContextPriority = 1;

	float BaseEyeZ;
	float CurrentEyeOffset;
	float TargetEyeOffset;
	float TargetCameraRoll;
	float CurrentCameraRoll;
	bool bIsCrouching;
	bool bWantsToMoveForward;

	USceneComponent* GetEyeComponent() const;
	void ApplyViewmodelInputContexts();

	// ---- Look Sensitivity ----
	// Global look multiplier on top of the raw axis. Tune live in PIE to
	// match your Apex cm/360 (1.0 = engine default).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Look", meta = (ClampMin = "0.01", ClampMax = "10.0"))
	float MouseSensitivity = 1.f;

	// ---- Input Handlers ----
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	void StartSprint();
	void StopSprint();

	void StartCrouch();
	void StopCrouch();

	void OnJumpPressed();
	void OnJumpReleased();

	void UpdateCameraTilt(float DeltaTime);

	// ---- Weapon ----
	void StartFire();
	void StopFire();
	void StartADS();
	void StopADS();
	void DrawWeapon();
	void StowWeapon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AALWeapon> WeaponClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AALWeapon> CurrentWeapon;

	void SpawnWeapon();

	// ---- Viewmodel (FPS Animation pack) ----

	// On-screen readout: active gun, fire mode, montage, movement state.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel|Debug")
	bool bShowViewmodelDebug = true;

	// Flip each newly equipped pack weapon to full auto (pack default is
	// per-weapon; we want auto as the baseline).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	bool bForceAutoFireMode = true;

	// Drive the pack's MovementState (E_MovementState) from our movement
	// component each tick, since its own sprint inputs are unmapped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	bool bDriveViewmodelMovementState = true;

	// Write the pack's DesiredSpeed (their gait treats it as movement intent,
	// not a normalizer - feeding our speed caps reads as permanent sprint).
	// Off: direct gait drive is the working setup.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	bool bFeedViewmodelDesiredSpeed = false;

	// Bypass their gait math entirely: write Gait (0=idle..1=walk..2=sprint..
	// 3=tac-sprint) straight onto the pawn and ViewmodelController.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	bool bFeedViewmodelGaitDirect = true;

	// Min ground speed before the arms play the sprint cycle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	float SprintAnimSpeedThreshold = 600.f;

	// Tac-sprint arms as a high-momentum reward. Disabled for now; sprint
	// band covers all fast movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	bool bEnableTacSprintAnim = false;

	// Momentum at which sprint arms upgrade to the tac-sprint pump.
	// Sprint alone builds ~30/s, so 85 keeps tac a sustained-speed reward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	float TacSprintMomentumThreshold = 85.f;

	// Eases gait band transitions; 0 = snap instantly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	float ViewmodelGaitInterpSpeed = 8.f;

	// Gait value at full sprint speed. The pack's anim BP reads ~2.0 as
	// tac-sprint, so the sprint band tops out just below it. Tune live until
	// the arms show the sprint cycle at 1200.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	float GaitSprintMax = 1.9f;

	// Gait value written for the momentum tac-sprint reward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel")
	float GaitTacValue = 3.f;

	float CurrentViewmodelGait = 0.f;

	void UpdateViewmodelMovementState();

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastFireModeWeapon;

	AActor* GetActiveViewmodelWeapon() const;
	void UpdateViewmodelDebug();

	// ---- Viewmodel Ballistics ----
	// Projectile spawned per shot of the pack's weapons (their fire is
	// animation-only). Hooked off the arms' Fire montage starting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel|Ballistics")
	TSubclassOf<AALProjectile> ViewmodelProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewmodel|Ballistics")
	float AimTraceDistance = 50000.f;

	TWeakObjectPtr<UAnimInstance> BoundArmsAnim;
	TWeakObjectPtr<USkeletalMeshComponent> CachedWeaponMesh;
	FName CachedMuzzleSocket;

	UFUNCTION()
	void OnArmsMontageStarted(UAnimMontage* Montage);
	void EnsureArmsMontageBinding();
	void FireViewmodelProjectile();
	USkeletalMeshComponent* FindViewmodelWeaponMesh() const;
};