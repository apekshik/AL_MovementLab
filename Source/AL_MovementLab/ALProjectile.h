// ALProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

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

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
