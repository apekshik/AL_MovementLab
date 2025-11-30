// ALCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALCharacter.generated.h"

class UCameraComponent;
class UALCharacterMovementComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float StandingCameraHeight = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float CrouchingCameraHeight = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	float CrouchCameraInterpSpeed = 8.f;

	float TargetCameraHeight;
	bool bIsCrouching;

	// ---- Input Handlers ----
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	void StartSprint();
	void StopSprint();

	void StartCrouch();
	void StopCrouch();
};