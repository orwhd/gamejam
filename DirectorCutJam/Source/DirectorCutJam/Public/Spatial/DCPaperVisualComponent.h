#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DCPaperVisualComponent.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

USTRUCT(BlueprintType)
struct DIRECTORCUTJAM_API FDCStateTextureSequence
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TArray<TObjectPtr<UTexture2D>> IdleFrames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TArray<TObjectPtr<UTexture2D>> MoveFrames;
};

/**
 * 2.5D world-space paper card visual component.
 * Origin is grounded at Bottom-Center (P_foot = local Z=0).
 * Keeps Pitch=0 and Roll=0 at all times, faces active camera plane, and flips horizontally on left/right movement.
 */
UCLASS(ClassGroup=(DirectorCut), meta=(BlueprintSpawnableComponent))
class DIRECTORCUTJAM_API UDCPaperVisualComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UDCPaperVisualComponent();

    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Paper2D")
    TObjectPtr<UPaperFlipbookComponent> FlipbookComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Paper2D")
    TObjectPtr<UPaperSpriteComponent> SpriteComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Paper2D")
    TObjectPtr<UStaticMeshComponent> CardMeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Paper2D")
    TObjectPtr<UStaticMeshComponent> BuddyCardMeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Paper2D")
    TObjectPtr<UStaticMeshComponent> ToolCardMeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Paper2D")
    TObjectPtr<UStaticMeshComponent> SprayCardMeshComp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Slots")
    TObjectPtr<UPaperFlipbook> IdleFlipbook;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Slots")
    TObjectPtr<UPaperFlipbook> MoveFlipbook;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Slots")
    TMap<FName, TObjectPtr<UPaperFlipbook>> StateIdleFlipbooks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Slots")
    TMap<FName, TObjectPtr<UPaperFlipbook>> StateMoveFlipbooks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Slots")
    TObjectPtr<UPaperSprite> IdleSprite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Slots")
    TMap<FName, TObjectPtr<UPaperSprite>> StateSprites;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox")
    TMap<FName, FLinearColor> StateTints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Material")
    TObjectPtr<UMaterialInterface> PaperMaterialOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TObjectPtr<UTexture2D> DefaultPaperTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TArray<TObjectPtr<UTexture2D>> IdleCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TArray<TObjectPtr<UTexture2D>> WalkCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TMap<FName, TObjectPtr<UTexture2D>> StateTextures;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TMap<FName, FDCStateTextureSequence> StateTextureSequences;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TObjectPtr<UTexture2D> ToolOverlayTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TObjectPtr<UTexture2D> SprayOverlayTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TObjectPtr<UTexture2D> BuddyUninjuredTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures")
    TObjectPtr<UTexture2D> BuddyInjuredTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Animation")
    bool bEnableIdleBreathing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Animation")
    bool bEnableWalkBounce = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures", meta=(ClampMin="0.1"))
    float IdleAnimFPS = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Textures", meta=(ClampMin="0.1"))
    float WalkAnimFPS = 8.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox", meta=(ClampMin="1.0"))
    FVector2D CardSizeCm = FVector2D(85.0f, 175.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox", meta=(ClampMin="1.0"))
    FVector2D DeadCardSizeCm = FVector2D(130.0f, 44.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox")
    FLinearColor GrayboxTint = FLinearColor(0.95f, 0.55f, 0.12f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox")
    FLinearColor BuddyUninjuredTint = FLinearColor(0.10f, 0.80f, 0.62f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox")
    FLinearColor BuddyInjuredTint = FLinearColor(0.95f, 0.65f, 0.15f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox")
    FLinearColor ToolAccentTint = FLinearColor(0.88f, 0.14f, 0.14f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Graybox")
    FLinearColor SprayAccentTint = FLinearColor(0.35f, 0.67f, 0.86f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Orientation")
    bool bFaceCameraPlane = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paper2D|Rendering")
    bool bCastDynamicShadow = true;

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void UpdateMovementFlip(const FVector& WorldVelocity, const FVector& CameraRightVector);

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void SetFacingLeft(bool bInFacingLeft);

    UFUNCTION(BlueprintPure, Category="Paper2D")
    bool IsFacingLeft() const { return bFacingLeft; }

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void SetPaperState(FName StateKey);

    UFUNCTION(BlueprintPure, Category="Paper2D")
    FName GetPaperState() const { return CurrentStateKey; }

    UFUNCTION(BlueprintPure, Category="Paper2D")
    bool HasCustomTextureForCurrentState() const;

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void SetGrayboxTint(const FLinearColor& NewTint);

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void SetCardSize(const FVector2D& NewSizeCm);

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void SetPaperVisible(bool bInVisible);

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void RefreshVisualAppearance();

    UFUNCTION(BlueprintCallable, Category="Paper2D")
    void FaceActiveCamera();

private:
    UStaticMeshComponent* CreatePlaneCardSubobject(const TCHAR* Name, UStaticMesh* PlaneMesh);
    void ApplyCardPlaneTransform(
        UStaticMeshComponent* MeshComp,
        const FVector2D& SizeCm,
        const FVector& LocalOffset,
        bool bUseSquareAspect = false,
        float HeightScale = 1.0f,
        float SwayRollDeg = 0.0f) const;
    void UpdateCardDynamicMaterial(
        UStaticMeshComponent* MeshComp,
        TObjectPtr<UMaterialInstanceDynamic>& DynMat,
        TObjectPtr<UMaterialInterface>& CachedMat,
        UMaterialInterface* BaseMat,
        const FLinearColor& TintColor,
        UTexture2D* BoundTexture = nullptr);

    void GetGrayboxStateFlags(
        bool& bOutDead,
        bool& bOutCombinedEscort,
        bool& bOutEscortInjured,
        bool& bOutShowTool,
        bool& bOutShowSpray) const;

    UTexture2D* ResolveTextureFromFrames(const TArray<TObjectPtr<UTexture2D>>& Frames, float FPS) const;
    UTexture2D* ResolveFallbackLocomotionTexture() const;
    UTexture2D* ResolveActiveCardTexture() const;
    UTexture2D* ResolveMainCardDisplayTexture() const;

    void ApplyCardTransforms();
    void ApplyCardMaterials();
    UMaterialInterface* ResolvePaperBaseMaterial() const;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicCardMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> CachedBaseMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicBuddyMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> CachedBuddyBaseMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicToolMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> CachedToolBaseMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicSprayMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> CachedSprayBaseMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> LastAppliedCardTexture;

    UPROPERTY(Transient)
    bool bFacingLeft = false;

    UPROPERTY(Transient)
    bool bIsMoving = false;

    UPROPERTY(Transient)
    float AnimTimeSeconds = 0.0f;

    UPROPERTY(Transient)
    FName CurrentStateKey = NAME_None;
};
