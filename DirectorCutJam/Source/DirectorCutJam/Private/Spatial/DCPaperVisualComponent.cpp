#include "Spatial/DCPaperVisualComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace DCPaperStateNames
{
    static const FName Dead(TEXT("Dead"));
    static const FName HoldingTool(TEXT("HoldingTool"));
    static const FName Spraying(TEXT("Spraying"));
    static const FName CombinedEscortUninjured(TEXT("CombinedEscortUninjured"));
    static const FName CombinedEscortInjured(TEXT("CombinedEscortInjured"));
    static const FName CombinedEscortUninjuredWithTool(TEXT("CombinedEscortUninjuredWithTool"));
    static const FName CombinedEscortInjuredWithTool(TEXT("CombinedEscortInjuredWithTool"));
}

UDCPaperVisualComponent::UDCPaperVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bTickInEditor = false;

    FlipbookComp = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComp"));
    FlipbookComp->SetupAttachment(this);
    FlipbookComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    FlipbookComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FlipbookComp->SetGenerateOverlapEvents(false);
    FlipbookComp->SetVisibility(false);

    SpriteComp = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComp"));
    SpriteComp->SetupAttachment(this);
    SpriteComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    SpriteComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpriteComp->SetGenerateOverlapEvents(false);
    SpriteComp->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
    UStaticMesh* PlaneMesh = PlaneMeshFinder.Succeeded() ? PlaneMeshFinder.Object : nullptr;

    CardMeshComp = CreatePlaneCardSubobject(TEXT("CardMeshComp"), PlaneMesh);
    CardMeshComp->SetVisibility(true);

    BuddyCardMeshComp = CreatePlaneCardSubobject(TEXT("BuddyCardMeshComp"), PlaneMesh);
    BuddyCardMeshComp->SetVisibility(false);

    ToolCardMeshComp = CreatePlaneCardSubobject(TEXT("ToolCardMeshComp"), PlaneMesh);
    ToolCardMeshComp->SetVisibility(false);

    SprayCardMeshComp = CreatePlaneCardSubobject(TEXT("SprayCardMeshComp"), PlaneMesh);
    SprayCardMeshComp->SetVisibility(false);

    SprayAccentTint = FLinearColor::FromSRGBColor(FColor(0x9E, 0xD4, 0xEE, 0xFF));

    ApplyCardTransforms();
}

UStaticMeshComponent* UDCPaperVisualComponent::CreatePlaneCardSubobject(const TCHAR* Name, UStaticMesh* PlaneMesh)
{
    UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Comp->SetupAttachment(this);
    if (PlaneMesh)
    {
        Comp->SetStaticMesh(PlaneMesh);
    }
    Comp->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Comp->SetGenerateOverlapEvents(false);
    return Comp;
}

void UDCPaperVisualComponent::OnRegister()
{
    Super::OnRegister();

    ApplyCardTransforms();
    if (!IsTemplate())
    {
        RefreshVisualAppearance();
        FaceActiveCamera();
    }
}

void UDCPaperVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    ApplyCardTransforms();
    RefreshVisualAppearance();
    FaceActiveCamera();
}

void UDCPaperVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AnimTimeSeconds += FMath::Max(0.0f, DeltaTime);
    FaceActiveCamera();

    bool bIsDead = false;
    bool bIsCombinedEscort = false;
    bool bIsEscortInjured = false;
    bool bShowTool = false;
    bool bShowSpray = false;
    GetGrayboxStateFlags(bIsDead, bIsCombinedEscort, bIsEscortInjured, bShowTool, bShowSpray);

    UTexture2D* DesiredTexture = ResolveMainCardDisplayTexture();
    if (DesiredTexture != LastAppliedCardTexture.Get())
    {
        ApplyCardMaterials();
        ApplyCardTransforms();
    }
    else if (IsVisible() && (bIsMoving || (bEnableIdleBreathing && !bIsDead)))
    {
        ApplyCardTransforms();
    }
}

void UDCPaperVisualComponent::FaceActiveCamera()
{
    if (!bFaceCameraPlane)
    {
        return;
    }

    float CameraYaw = 1.5f;
    bool bFoundCamera = false;

    if (const UWorld* World = GetWorld())
    {
        if (const APlayerController* PC = World->GetFirstPlayerController())
        {
            if (PC->PlayerCameraManager)
            {
                CameraYaw = PC->PlayerCameraManager->GetCameraRotation().Yaw;
                bFoundCamera = true;
            }
            else if (const AActor* ViewTarget = PC->GetViewTarget())
            {
                CameraYaw = ViewTarget->GetActorRotation().Yaw;
                bFoundCamera = true;
            }
        }

        if (!bFoundCamera)
        {
            for (TActorIterator<ACameraActor> CamIt(const_cast<UWorld*>(World)); CamIt; ++CamIt)
            {
                if (const ACameraActor* CamActor = *CamIt)
                {
                    CameraYaw = CamActor->GetActorRotation().Yaw;
                    bFoundCamera = true;
                    break;
                }
            }
        }
    }

    SetWorldRotation(FRotator(0.0f, CameraYaw + 180.0f, 0.0f));
}

void UDCPaperVisualComponent::UpdateMovementFlip(const FVector& WorldVelocity, const FVector& CameraRightVector)
{
    const FVector HorizVelocity(WorldVelocity.X, WorldVelocity.Y, 0.0f);
    const FVector HorizRight = FVector(CameraRightVector.X, CameraRightVector.Y, 0.0f).GetSafeNormal();

    if (!HorizRight.IsNearlyZero())
    {
        const float RightDot = FVector::DotProduct(HorizVelocity, HorizRight);
        if (RightDot > 5.0f)
        {
            SetFacingLeft(false);
        }
        else if (RightDot < -5.0f)
        {
            SetFacingLeft(true);
        }
    }

    const bool bNowMoving = HorizVelocity.SizeSquared() > 25.0f;
    if (bIsMoving != bNowMoving)
    {
        bIsMoving = bNowMoving;
        RefreshVisualAppearance();
    }
}

void UDCPaperVisualComponent::SetFacingLeft(bool bInFacingLeft)
{
    if (bFacingLeft != bInFacingLeft)
    {
        bFacingLeft = bInFacingLeft;
        ApplyCardTransforms();
        ApplyCardMaterials();
    }
}

void UDCPaperVisualComponent::SetPaperState(FName StateKey)
{
    if (CurrentStateKey != StateKey)
    {
        CurrentStateKey = StateKey;
        RefreshVisualAppearance();
    }
}

void UDCPaperVisualComponent::SetGrayboxTint(const FLinearColor& NewTint)
{
    GrayboxTint = NewTint;
    ApplyCardMaterials();
}

void UDCPaperVisualComponent::SetCardSize(const FVector2D& NewSizeCm)
{
    CardSizeCm.X = FMath::Max(1.0f, NewSizeCm.X);
    CardSizeCm.Y = FMath::Max(1.0f, NewSizeCm.Y);
    ApplyCardTransforms();
}

void UDCPaperVisualComponent::SetPaperVisible(bool bInVisible)
{
    if (bInVisible)
    {
        SetVisibility(true, false);
        RefreshVisualAppearance();
    }
    else
    {
        SetVisibility(false, true);
    }
}

void UDCPaperVisualComponent::GetGrayboxStateFlags(
    bool& bOutDead,
    bool& bOutCombinedEscort,
    bool& bOutEscortInjured,
    bool& bOutShowTool,
    bool& bOutShowSpray) const
{
    bOutDead = (CurrentStateKey == DCPaperStateNames::Dead);
    bOutCombinedEscort =
        (CurrentStateKey == DCPaperStateNames::CombinedEscortUninjured) ||
        (CurrentStateKey == DCPaperStateNames::CombinedEscortInjured) ||
        (CurrentStateKey == DCPaperStateNames::CombinedEscortUninjuredWithTool) ||
        (CurrentStateKey == DCPaperStateNames::CombinedEscortInjuredWithTool);
    bOutEscortInjured =
        (CurrentStateKey == DCPaperStateNames::CombinedEscortInjured) ||
        (CurrentStateKey == DCPaperStateNames::CombinedEscortInjuredWithTool);
    bOutShowTool =
        (CurrentStateKey == DCPaperStateNames::HoldingTool) ||
        (CurrentStateKey == DCPaperStateNames::Spraying) ||
        (CurrentStateKey == DCPaperStateNames::CombinedEscortUninjuredWithTool) ||
        (CurrentStateKey == DCPaperStateNames::CombinedEscortInjuredWithTool);
    bOutShowSpray = (CurrentStateKey == DCPaperStateNames::Spraying);
}

bool UDCPaperVisualComponent::HasCustomTextureForCurrentState() const
{
    return ResolveActiveCardTexture() != nullptr;
}

UTexture2D* UDCPaperVisualComponent::ResolveTextureFromFrames(const TArray<TObjectPtr<UTexture2D>>& Frames, float FPS) const
{
    if (Frames.Num() == 0)
    {
        return nullptr;
    }

    if (Frames.Num() == 1)
    {
        return Frames[0].Get();
    }

    const float SafeFPS = FMath::Max(0.1f, FPS);
    const int32 RawIdx = FMath::FloorToInt(FMath::Max(0.0f, AnimTimeSeconds) * SafeFPS);
    const int32 FrameIdx = RawIdx % Frames.Num();
    if (UTexture2D* DirectTex = Frames[FrameIdx].Get())
    {
        return DirectTex;
    }

    for (const TObjectPtr<UTexture2D>& Candidate : Frames)
    {
        if (Candidate)
        {
            return Candidate.Get();
        }
    }

    return nullptr;
}

UTexture2D* UDCPaperVisualComponent::ResolveActiveCardTexture() const
{
    bool bIsDead = false;
    bool bIsCombinedEscort = false;
    bool bIsEscortInjured = false;
    bool bShowTool = false;
    bool bShowSpray = false;
    GetGrayboxStateFlags(bIsDead, bIsCombinedEscort, bIsEscortInjured, bShowTool, bShowSpray);

    if (CurrentStateKey != NAME_None)
    {
        if (const FDCStateTextureSequence* Seq = StateTextureSequences.Find(CurrentStateKey))
        {
            const TArray<TObjectPtr<UTexture2D>>& PrimaryFrames = bIsMoving ? Seq->MoveFrames : Seq->IdleFrames;
            const float PrimaryFPS = bIsMoving ? WalkAnimFPS : IdleAnimFPS;
            if (UTexture2D* PrimaryTex = ResolveTextureFromFrames(PrimaryFrames, PrimaryFPS))
            {
                return PrimaryTex;
            }

            const TArray<TObjectPtr<UTexture2D>>& SecondaryFrames = bIsMoving ? Seq->IdleFrames : Seq->MoveFrames;
            const float SecondaryFPS = bIsMoving ? IdleAnimFPS : WalkAnimFPS;
            if (UTexture2D* SecondaryTex = ResolveTextureFromFrames(SecondaryFrames, SecondaryFPS))
            {
                return SecondaryTex;
            }
        }

        if (const TObjectPtr<UTexture2D>* StateTex = StateTextures.Find(CurrentStateKey))
        {
            if (StateTex->Get())
            {
                return StateTex->Get();
            }
        }

        if (bIsCombinedEscort)
        {
            const FName BaseEscortState = bIsEscortInjured
                ? DCPaperStateNames::CombinedEscortInjured
                : DCPaperStateNames::CombinedEscortUninjured;
            if (CurrentStateKey != BaseEscortState)
            {
                if (const FDCStateTextureSequence* BaseSeq = StateTextureSequences.Find(BaseEscortState))
                {
                    const TArray<TObjectPtr<UTexture2D>>& PrimaryFrames = bIsMoving ? BaseSeq->MoveFrames : BaseSeq->IdleFrames;
                    const float PrimaryFPS = bIsMoving ? WalkAnimFPS : IdleAnimFPS;
                    if (UTexture2D* PrimaryTex = ResolveTextureFromFrames(PrimaryFrames, PrimaryFPS))
                    {
                        return PrimaryTex;
                    }
                    const TArray<TObjectPtr<UTexture2D>>& SecondaryFrames = bIsMoving ? BaseSeq->IdleFrames : BaseSeq->MoveFrames;
                    const float SecondaryFPS = bIsMoving ? IdleAnimFPS : WalkAnimFPS;
                    if (UTexture2D* SecondaryTex = ResolveTextureFromFrames(SecondaryFrames, SecondaryFPS))
                    {
                        return SecondaryTex;
                    }
                }
                if (const TObjectPtr<UTexture2D>* BaseStateTex = StateTextures.Find(BaseEscortState))
                {
                    if (BaseStateTex->Get())
                    {
                        return BaseStateTex->Get();
                    }
                }
            }
            return nullptr;
        }
    }

    return ResolveFallbackLocomotionTexture();
}

UTexture2D* UDCPaperVisualComponent::ResolveFallbackLocomotionTexture() const
{
    if (bIsMoving)
    {
        if (UTexture2D* WalkTex = ResolveTextureFromFrames(WalkCycleTextures, WalkAnimFPS))
        {
            return WalkTex;
        }
    }

    if (UTexture2D* IdleTex = ResolveTextureFromFrames(IdleCycleTextures, IdleAnimFPS))
    {
        return IdleTex;
    }

    if (!bIsMoving)
    {
        if (UTexture2D* WalkFallbackTex = ResolveTextureFromFrames(WalkCycleTextures, WalkAnimFPS))
        {
            return WalkFallbackTex;
        }
    }

    return DefaultPaperTexture.Get();
}

UTexture2D* UDCPaperVisualComponent::ResolveMainCardDisplayTexture() const
{
    if (UTexture2D* DirectTex = ResolveActiveCardTexture())
    {
        return DirectTex;
    }

    bool bIsDead = false;
    bool bIsCombinedEscort = false;
    bool bIsEscortInjured = false;
    bool bShowTool = false;
    bool bShowSpray = false;
    GetGrayboxStateFlags(bIsDead, bIsCombinedEscort, bIsEscortInjured, bShowTool, bShowSpray);
    return bIsCombinedEscort ? ResolveFallbackLocomotionTexture() : nullptr;
}

void UDCPaperVisualComponent::ApplyCardPlaneTransform(
    UStaticMeshComponent* MeshComp,
    const FVector2D& SizeCm,
    const FVector& LocalOffset,
    bool bUseSquareAspect,
    float HeightScale,
    float SwayRollDeg) const
{
    if (!MeshComp)
    {
        return;
    }

    const float SafeWidth = FMath::Max(1.0f, SizeCm.X);
    const float SafeHeight = FMath::Max(1.0f, SizeCm.Y);
    const float SquareSide = FMath::Max(SafeWidth, SafeHeight);
    const float EffectiveWidth = bUseSquareAspect ? SquareSide : SafeWidth;
    const float EffectiveHeight = (bUseSquareAspect ? SquareSide : SafeHeight) * FMath::Max(0.1f, HeightScale);

    // Keep Plane front normal facing camera with strictly positive scale; mirror horizontally via FlipU param.
    // Tilt sway rotates purely within the card's upright vertical plane around local Forward (+X).
    const FQuat BasePlaneQuat(FRotator(-90.0f, 0.0f, 0.0f));
    const FQuat SwayQuat(FVector::ForwardVector, FMath::DegreesToRadians(SwayRollDeg));
    MeshComp->SetRelativeRotation((SwayQuat * BasePlaneQuat).Rotator());
    MeshComp->SetRelativeLocation(FVector(LocalOffset.X, LocalOffset.Y, LocalOffset.Z + 0.5f * EffectiveHeight));
    MeshComp->SetRelativeScale3D(FVector(EffectiveHeight / 100.0f, EffectiveWidth / 100.0f, 1.0f));
    MeshComp->SetCastShadow(bCastDynamicShadow);
}

void UDCPaperVisualComponent::ApplyCardTransforms()
{
    bool bIsDead = false;
    bool bIsCombinedEscort = false;
    bool bIsEscortInjured = false;
    bool bShowTool = false;
    bool bShowSpray = false;
    GetGrayboxStateFlags(bIsDead, bIsCombinedEscort, bIsEscortInjured, bShowTool, bShowSpray);

    const bool bHasCustomCardTex = HasCustomTextureForCurrentState();
    const bool bHasCustomCombinedTexture = bIsCombinedEscort && bHasCustomCardTex;
    const bool bHasDisplayTexture = (ResolveMainCardDisplayTexture() != nullptr);
    const float FlipSign = bFacingLeft ? 1.0f : -1.0f;

    const float BreathHeightScale = (bEnableIdleBreathing && !bIsDead)
        ? (1.0f + 0.016f * FMath::Sin(AnimTimeSeconds * 3.2f))
        : 1.0f;
    const float WalkBounceZ = (bEnableWalkBounce && bIsMoving && !bIsDead)
        ? FMath::Abs(FMath::Sin(AnimTimeSeconds * 14.0f)) * 4.5f
        : 0.0f;
    const float WalkSwayDeg = (bEnableWalkBounce && bIsMoving && !bIsDead)
        ? (FMath::Sin(AnimTimeSeconds * 7.0f) * 2.8f)
        : 0.0f;

    const FVector2D EffectiveCardSize = bIsDead ? DeadCardSizeCm : CardSizeCm;
    const FVector MainOffset = (bIsCombinedEscort && !bHasCustomCombinedTexture)
        ? FVector(0.0f, -FlipSign * 18.0f, WalkBounceZ)
        : FVector(0.0f, 0.0f, WalkBounceZ);
    ApplyCardPlaneTransform(
        CardMeshComp,
        EffectiveCardSize,
        MainOffset,
        bHasDisplayTexture,
        BreathHeightScale,
        WalkSwayDeg);

    if (BuddyCardMeshComp)
    {
        const float BuddyHeight = bIsEscortInjured ? 142.0f : 156.0f;
        const FVector2D BuddySize(66.0f, BuddyHeight);
        UTexture2D* ActiveBuddyTex = bIsEscortInjured ? BuddyInjuredTexture.Get() : BuddyUninjuredTexture.Get();
        ApplyCardPlaneTransform(
            BuddyCardMeshComp,
            BuddySize,
            FVector(1.5f, FlipSign * 26.0f, WalkBounceZ),
            ActiveBuddyTex != nullptr,
            BreathHeightScale,
            WalkSwayDeg);
    }

    if (ToolCardMeshComp)
    {
        const float ToolLateralY = FlipSign * (bIsCombinedEscort ? 48.0f : 34.0f);
        ApplyCardPlaneTransform(
            ToolCardMeshComp,
            FVector2D(24.0f, 64.0f),
            FVector(3.0f, ToolLateralY, WalkBounceZ),
            ToolOverlayTexture != nullptr,
            1.0f,
            WalkSwayDeg);
    }

    if (SprayCardMeshComp)
    {
        const bool bHasSprayTex = (SprayOverlayTexture != nullptr);
        const float SprayLateralY = FlipSign * (bHasSprayTex ? 82.0f : 68.0f);
        const FVector2D SpraySize = bHasSprayTex ? FVector2D(84.0f, 84.0f) : FVector2D(58.0f, 26.0f);
        const float SprayZ = (bHasSprayTex ? 44.0f : 64.0f) + WalkBounceZ;
        ApplyCardPlaneTransform(SprayCardMeshComp, SpraySize, FVector(3.5f, SprayLateralY, SprayZ), bHasSprayTex, 1.0f, 0.0f);
    }

    const float FlipScalar = bFacingLeft ? -1.0f : 1.0f;
    if (DynamicCardMaterial) { DynamicCardMaterial->SetScalarParameterValue(TEXT("FlipU"), FlipScalar); }
    if (DynamicBuddyMaterial) { DynamicBuddyMaterial->SetScalarParameterValue(TEXT("FlipU"), FlipScalar); }
    if (DynamicToolMaterial) { DynamicToolMaterial->SetScalarParameterValue(TEXT("FlipU"), FlipScalar); }
    if (DynamicSprayMaterial) { DynamicSprayMaterial->SetScalarParameterValue(TEXT("FlipU"), FlipScalar); }

    const float SpriteYaw = bFacingLeft ? 90.0f : -90.0f;

    if (SpriteComp)
    {
        SpriteComp->SetRelativeRotation(FRotator(0.0f, SpriteYaw, 0.0f));
        SpriteComp->SetRelativeLocation(FVector::ZeroVector);
        SpriteComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
        SpriteComp->SetCastShadow(bCastDynamicShadow);
    }

    if (FlipbookComp)
    {
        FlipbookComp->SetRelativeRotation(FRotator(0.0f, SpriteYaw, 0.0f));
        FlipbookComp->SetRelativeLocation(FVector::ZeroVector);
        FlipbookComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
        FlipbookComp->SetCastShadow(bCastDynamicShadow);
    }
}

UMaterialInterface* UDCPaperVisualComponent::ResolvePaperBaseMaterial() const
{
    if (PaperMaterialOverride)
    {
        return PaperMaterialOverride.Get();
    }

    if (UMaterialInterface* ProjectMasked = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_PaperMasked.M_PaperMasked")))
    {
        return ProjectMasked;
    }

    if (UMaterialInterface* PaperMasked = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial")))
    {
        return PaperMasked;
    }

    return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

void UDCPaperVisualComponent::UpdateCardDynamicMaterial(
    UStaticMeshComponent* MeshComp,
    TObjectPtr<UMaterialInstanceDynamic>& DynMat,
    TObjectPtr<UMaterialInterface>& CachedMat,
    UMaterialInterface* BaseMat,
    const FLinearColor& TintColor,
    UTexture2D* BoundTexture)
{
    if (!MeshComp || !BaseMat)
    {
        return;
    }

    if (!DynMat || CachedMat != BaseMat)
    {
        DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
        CachedMat = BaseMat;
        MeshComp->SetMaterial(0, DynMat);
    }

    if (DynMat)
    {
        UTexture* BaseTexture = nullptr;
        const bool bHasPaperTextureParam = BaseMat->GetTextureParameterValue(FMaterialParameterInfo(TEXT("PaperTexture")), BaseTexture);
        if (BoundTexture)
        {
            if (bHasPaperTextureParam)
            {
                DynMat->SetTextureParameterValue(TEXT("PaperTexture"), BoundTexture);
            }
        }
        else if (bHasPaperTextureParam && BaseTexture)
        {
            DynMat->SetTextureParameterValue(TEXT("PaperTexture"), BaseTexture);
        }

        const FLinearColor AppliedTint = BoundTexture ? FLinearColor::White : TintColor;
        FLinearColor DummyColor;
        if (BaseMat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("TintColor")), DummyColor))
        {
            DynMat->SetVectorParameterValue(TEXT("TintColor"), AppliedTint);
        }
        if (BaseMat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), DummyColor))
        {
            DynMat->SetVectorParameterValue(TEXT("Color"), AppliedTint);
        }
        DynMat->SetScalarParameterValue(TEXT("FlipU"), bFacingLeft ? -1.0f : 1.0f);
    }
}

void UDCPaperVisualComponent::ApplyCardMaterials()
{
    if (IsTemplate())
    {
        return;
    }

    bool bIsDead = false;
    bool bIsCombinedEscort = false;
    bool bIsEscortInjured = false;
    bool bShowTool = false;
    bool bShowSpray = false;
    GetGrayboxStateFlags(bIsDead, bIsCombinedEscort, bIsEscortInjured, bShowTool, bShowSpray);

    FLinearColor EffectiveTint = GrayboxTint;
    if (CurrentStateKey != NAME_None)
    {
        if (const FLinearColor* FoundTint = StateTints.Find(CurrentStateKey))
        {
            EffectiveTint = *FoundTint;
        }
        else if (bIsDead)
        {
            EffectiveTint = FLinearColor::FromSRGBColor(FColor(0x6B, 0x62, 0x5C, 0xFF));
        }
    }

    UTexture2D* ActiveCardTexture = ResolveMainCardDisplayTexture();
    LastAppliedCardTexture = ActiveCardTexture;
    UTexture2D* ActiveBuddyTexture = bIsEscortInjured
        ? BuddyInjuredTexture.Get()
        : BuddyUninjuredTexture.Get();

    if (UMaterialInterface* BaseMat = ResolvePaperBaseMaterial())
    {
        UpdateCardDynamicMaterial(
            CardMeshComp,
            DynamicCardMaterial,
            CachedBaseMaterial,
            BaseMat,
            EffectiveTint,
            ActiveCardTexture);
        UpdateCardDynamicMaterial(
            BuddyCardMeshComp,
            DynamicBuddyMaterial,
            CachedBuddyBaseMaterial,
            BaseMat,
            bIsEscortInjured ? BuddyInjuredTint : BuddyUninjuredTint,
            ActiveBuddyTexture);
        UpdateCardDynamicMaterial(
            ToolCardMeshComp,
            DynamicToolMaterial,
            CachedToolBaseMaterial,
            BaseMat,
            ToolAccentTint,
            ToolOverlayTexture.Get());
        UpdateCardDynamicMaterial(
            SprayCardMeshComp,
            DynamicSprayMaterial,
            CachedSprayBaseMaterial,
            BaseMat,
            SprayAccentTint,
            SprayOverlayTexture.Get());
    }

    if (SpriteComp)
    {
        if (PaperMaterialOverride)
        {
            SpriteComp->SetMaterial(0, PaperMaterialOverride);
        }
        const FLinearColor* StateTint = (CurrentStateKey != NAME_None) ? StateTints.Find(CurrentStateKey) : nullptr;
        SpriteComp->SetSpriteColor(StateTint ? *StateTint : FLinearColor::White);
    }

    if (FlipbookComp)
    {
        if (PaperMaterialOverride)
        {
            FlipbookComp->SetMaterial(0, PaperMaterialOverride);
        }
        const FLinearColor* StateTint = (CurrentStateKey != NAME_None) ? StateTints.Find(CurrentStateKey) : nullptr;
        FlipbookComp->SetSpriteColor(StateTint ? *StateTint : FLinearColor::White);
    }
}

void UDCPaperVisualComponent::RefreshVisualAppearance()
{
    ApplyCardMaterials();
    ApplyCardTransforms();

    UPaperFlipbook* ActiveFlipbook = nullptr;
    UPaperSprite* ActiveSprite = nullptr;

    if (CurrentStateKey != NAME_None)
    {
        if (bIsMoving)
        {
            if (const TObjectPtr<UPaperFlipbook>* FoundMoveFB = StateMoveFlipbooks.Find(CurrentStateKey))
            {
                ActiveFlipbook = FoundMoveFB->Get();
            }
        }

        if (!ActiveFlipbook)
        {
            if (const TObjectPtr<UPaperFlipbook>* FoundIdleFB = StateIdleFlipbooks.Find(CurrentStateKey))
            {
                ActiveFlipbook = FoundIdleFB->Get();
            }
        }

        if (!ActiveFlipbook)
        {
            if (const TObjectPtr<UPaperSprite>* FoundStateSprite = StateSprites.Find(CurrentStateKey))
            {
                ActiveSprite = FoundStateSprite->Get();
            }
        }

        if (!ActiveFlipbook && !ActiveSprite && !bIsMoving)
        {
            if (const TObjectPtr<UPaperFlipbook>* FoundMoveFallback = StateMoveFlipbooks.Find(CurrentStateKey))
            {
                ActiveFlipbook = FoundMoveFallback->Get();
            }
        }

        if (!ActiveFlipbook && !ActiveSprite && CurrentStateKey == TEXT("BlockEntranceArmsSpread"))
        {
            ActiveFlipbook = (bIsMoving && MoveFlipbook) ? MoveFlipbook.Get() : IdleFlipbook.Get();
            if (!ActiveFlipbook)
            {
                ActiveSprite = IdleSprite.Get();
            }
        }
    }
    else
    {
        ActiveFlipbook = (bIsMoving && MoveFlipbook) ? MoveFlipbook.Get() : IdleFlipbook.Get();
        if (!ActiveFlipbook)
        {
            ActiveSprite = IdleSprite.Get();
        }
    }

    const bool bUseFlipbook = (ActiveFlipbook != nullptr);
    const bool bUseSprite = (!bUseFlipbook && ActiveSprite != nullptr);
    const bool bUseGrayboxCard = (!bUseFlipbook && !bUseSprite);

    if (SpriteComp)
    {
        if (bUseSprite)
        {
            SpriteComp->SetSprite(ActiveSprite);
        }
        SpriteComp->SetVisibility(bUseSprite);
    }

    if (FlipbookComp)
    {
        if (bUseFlipbook)
        {
            if (FlipbookComp->GetFlipbook() != ActiveFlipbook)
            {
                FlipbookComp->SetFlipbook(ActiveFlipbook);
            }
            if (!FlipbookComp->IsPlaying())
            {
                FlipbookComp->Play();
            }
        }
        else
        {
            FlipbookComp->Stop();
        }
        FlipbookComp->SetVisibility(bUseFlipbook);
    }

    bool bIsDead = false;
    bool bIsCombinedEscort = false;
    bool bIsEscortInjured = false;
    bool bShowTool = false;
    bool bShowSpray = false;
    GetGrayboxStateFlags(bIsDead, bIsCombinedEscort, bIsEscortInjured, bShowTool, bShowSpray);

    const bool bHasCustomCardTex = (ResolveActiveCardTexture() != nullptr);
    const bool bHasCustomCombinedTexture = bIsCombinedEscort && bHasCustomCardTex;
    const bool bParentVisible = IsVisible();

    if (CardMeshComp)
    {
        CardMeshComp->SetVisibility(bParentVisible && bUseGrayboxCard);
    }
    if (BuddyCardMeshComp)
    {
        BuddyCardMeshComp->SetVisibility(bParentVisible && bUseGrayboxCard && bIsCombinedEscort && !bHasCustomCombinedTexture);
    }
    if (ToolCardMeshComp)
    {
        ToolCardMeshComp->SetVisibility(bParentVisible && bUseGrayboxCard && bShowTool && (!bHasCustomCardTex || ToolOverlayTexture != nullptr));
    }
    if (SprayCardMeshComp)
    {
        SprayCardMeshComp->SetVisibility(bParentVisible && bUseGrayboxCard && bShowSpray && (!bHasCustomCardTex || SprayOverlayTexture != nullptr));
    }
}
