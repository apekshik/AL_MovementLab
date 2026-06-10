// ALPuffEffect.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALPuffEffect.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

/**
 * One-shot expanding puff: spawns small, expands rapidly with an ease-out,
 * fades to nothing and destroys itself. Used for muzzle flash (hot orange,
 * with a light pulse) and impact smoke (gray, drifting). Configure the public
 * fields between SpawnActorDeferred and FinishSpawning.
 */
UCLASS()
class AL_MOVEMENTLAB_API AALPuffEffect : public AActor
{
	GENERATED_BODY()

public:
	AALPuffEffect();

	virtual void Tick(float DeltaTime) override;

	/** Total lifetime in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	float Duration;

	/** Scale at spawn (engine sphere is 100cm, so 0.1 = 10cm puff) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	float StartScale;

	/** Scale at end of life */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	float EndScale;

	/** Per-axis shape multiplier; stretch X for a streak, leave (1,1,1) for a ball */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	FVector ShapeScale;

	/** Emissive color (HDR values bloom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	FLinearColor Color;

	/** Peak opacity, 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	float OpacityScale;

	/** World-space drift in cm/sec (e.g. smoke rising) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	FVector DriftVelocity;

	/** Point light intensity at spawn; 0 disables the light */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	float LightIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	FLinearColor LightColor;

	/** Sphere mesh override; defaults to the engine basic sphere */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puff")
	TObjectPtr<UStaticMesh> PuffMesh;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puff")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puff")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID;

	float Age;
};
