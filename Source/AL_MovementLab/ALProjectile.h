// ALProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class AL_MOVEMENTLAB_API AALProjectile : public AActor
{
	GENERATED_BODY()

public:
	AALProjectile();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	/** Initialize velocity in the given direction */
	void FireInDirection(const FVector& ShootDirection);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Projectile speed in units/sec */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ProjectileSpeed;

	/** Gravity scale for bullet drop (1.0 = normal gravity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ProjectileGravityScale;

	/** Damage dealt on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage;

	/** Lifetime before auto-destroy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float LifeSpan;

	/** Stretch the visual mesh into a tracer streak along the flight direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Tracer")
	bool bScaleMeshAsTracer;

	/** Tracer length in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Tracer")
	float TracerLength;

	/** Tracer thickness in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Tracer")
	float TracerThickness;

	/** Material applied to the tracer mesh (defaults to /Game/Weapons/Effects/M_Tracer if unset) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Tracer")
	TObjectPtr<UMaterialInterface> TracerMaterial;

	/** Decal stamped where the projectile hits (defaults to /Game/Weapons/Effects/M_BulletHoleDecal if unset) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	TObjectPtr<UMaterialInterface> ImpactDecalMaterial;

	/** Bullet hole size in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	float ImpactDecalSize;

	/** Seconds before a bullet hole disappears */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	float ImpactDecalLifeSpan;

	/** Oldest bullet holes are removed once more than this many exist */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	int32 MaxBulletHoles;

	/** Spawn a small smoke puff at the impact point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	bool bImpactPuff;

	/** Overall impact puff size multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Impact")
	float ImpactPuffScale;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
