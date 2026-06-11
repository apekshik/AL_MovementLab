// ALProjectile.cpp

#include "ALProjectile.h"
#include "ALPuffEffect.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

// All bullet holes in spawn order, so the oldest can be removed once the cap
// is hit. Weak pointers: entries also die via decal lifespan or level change.
static TArray<TWeakObjectPtr<UDecalComponent>> GBulletHoleDecals;

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

	// Tracer look (bRotationFollowsVelocity keeps X aligned with travel direction)
	bScaleMeshAsTracer = true;
	TracerLength = 120.f;
	TracerThickness = 2.5f;

	// Impact decal
	ImpactDecalSize = 8.f;
	ImpactDecalLifeSpan = 60.f;
	MaxBulletHoles = 300;

	// Impact puff
	bImpactPuff = true;
	ImpactPuffScale = 1.f;

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

	// Stretch whatever mesh is assigned into a tracer streak, using its actual
	// bounds so the result is the same regardless of the source asset's size
	if (bScaleMeshAsTracer && MeshComponent && MeshComponent->GetStaticMesh())
	{
		const FVector MeshSize = MeshComponent->GetStaticMesh()->GetBounds().BoxExtent * 2.f;
		if (MeshSize.GetMin() > KINDA_SMALL_NUMBER)
		{
			MeshComponent->SetRelativeScale3D(FVector(
				TracerLength / MeshSize.X,
				TracerThickness / MeshSize.Y,
				TracerThickness / MeshSize.Z));
		}
	}

	// Bind hit event
	CollisionComponent->OnComponentHit.AddDynamic(this, &AALProjectile::OnHit);

	// Never collide with whoever fired us - the spawn point can sit inside
	// the owning pawn's capsule
	if (AActor* MyOwner = GetOwner())
	{
		CollisionComponent->IgnoreActorWhenMoving(MyOwner, true);
	}

	// Default content fallbacks, kept out of the constructor so a missing
	// asset can never break CDO construction; BP-assigned values win
	if (!TracerMaterial)
	{
		TracerMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weapons/Effects/M_Tracer.M_Tracer"));
	}
	if (!ImpactDecalMaterial)
	{
		ImpactDecalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weapons/Effects/M_BulletHoleDecal.M_BulletHoleDecal"));
	}

	if (TracerMaterial && MeshComponent)
	{
		MeshComponent->SetMaterial(0, TracerMaterial);
	}
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

		// Stamp a bullet hole on whatever we hit
		if (ImpactDecalMaterial)
		{
			FRotator DecalRotation = Hit.ImpactNormal.Rotation();
			DecalRotation.Roll = FMath::FRandRange(0.f, 360.f);

			UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(),
				ImpactDecalMaterial,
				FVector(4.f, ImpactDecalSize, ImpactDecalSize),
				Hit.ImpactPoint,
				DecalRotation,
				ImpactDecalLifeSpan);

			if (Decal)
			{
				// Don't cull bullet holes until they're tiny on screen
				Decal->SetFadeScreenSize(0.001f);

				// Enforce the global cap, oldest first
				GBulletHoleDecals.RemoveAll([](const TWeakObjectPtr<UDecalComponent>& D) { return !D.IsValid(); });
				GBulletHoleDecals.Add(Decal);
				while (GBulletHoleDecals.Num() > MaxBulletHoles)
				{
					if (UDecalComponent* Oldest = GBulletHoleDecals[0].Get())
					{
						Oldest->DestroyComponent();
					}
					GBulletHoleDecals.RemoveAt(0);
				}
			}
		}

		// Small smoke puff drifting off the surface, size slightly randomized
		if (bImpactPuff)
		{
			const float SizeRand = ImpactPuffScale * FMath::FRandRange(0.7f, 1.4f);
			const FTransform PuffTransform(FRotator::ZeroRotator, Hit.ImpactPoint + Hit.ImpactNormal * 5.f);
			if (AALPuffEffect* Puff = GetWorld()->SpawnActorDeferred<AALPuffEffect>(AALPuffEffect::StaticClass(), PuffTransform))
			{
				Puff->Duration = FMath::FRandRange(0.45f, 0.7f);
				Puff->StartScale = 0.07f * SizeRand;
				Puff->EndScale = 0.28f * SizeRand;
				Puff->Color = FLinearColor(0.4f, 0.38f, 0.36f);
				Puff->OpacityScale = 0.45f;
				Puff->DriftVelocity = Hit.ImpactNormal * 25.f + FVector(0.f, 0.f, 15.f);
				Puff->FinishSpawning(PuffTransform);
			}
		}

	}

	// A blocked projectile is spent no matter what stopped it - never leave
	// a frozen tracer hanging in the air
	Destroy();
}
