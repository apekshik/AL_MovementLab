// ALProjectile.cpp

#include "ALProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AALProjectile::AALProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision sphere
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(3.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	RootComponent = CollisionComponent;

	// Visual mesh (small sphere as tracer)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.05f));

	// Projectile movement component handles physics
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	// Default projectile settings (AR-like)
	ProjectileSpeed = 18000.f;  // Fast but not instant
	ProjectileGravityScale = 0.3f;  // Slight bullet drop
	Damage = 18.f;
	LifeSpan = 5.f;

	// Apply to movement component
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->ProjectileGravityScale = ProjectileGravityScale;
}

void AALProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Set lifespan
	SetLifeSpan(LifeSpan);

	// Bind hit event
	CollisionComponent->OnComponentHit.AddDynamic(this, &AALProjectile::OnHit);
}

void AALProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AALProjectile::FireInDirection(const FVector& ShootDirection)
{
	ProjectileMovement->Velocity = ShootDirection * ProjectileSpeed;
}

void AALProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Don't hit ourselves
	AActor* MyOwner = GetOwner();
	if (OtherActor && OtherActor != this && OtherActor != MyOwner)
	{
		// Apply damage
		UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, this, nullptr);

		// Destroy projectile
		Destroy();
	}
}
