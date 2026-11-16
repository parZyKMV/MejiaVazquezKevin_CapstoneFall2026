#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CookingComponent.generated.h"

class URecipeData;
class UItemData;
class UInventoryComponent;

UENUM(BlueprintType)
enum class ECookResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	InvalidRecipe UMETA(DisplayName = "Invalid Recipe"),
	UnknownRecipe UMETA(DisplayName = "Unknown Recipe"),
	MissingIngredients UMETA(DisplayName = "Missing Ingredients"),
	NoSpace UMETA(DisplayName = "No Space"),
	NoInventory UMETA(DisplayName = "No Inventory")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCookedSignature, URecipeData*, Recipe, UItemData*, Result, int32, Count);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCookFailedSignature, URecipeData*, Recipe, ECookResult, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecipeUnlockedSignature, URecipeData*, Recipe);

/**
 * Cocina del jugador.
 *
 * - Guarda las recetas que el jugador conoce (KnownRecipes).
 * - Cook() comprueba, gasta los ingredientes del InventoryComponent y entrega el resultado.
 * - No consume nada si falta un ingrediente o si el resultado no cabe en el inventario.
 * - Donde se puede cocinar (la estacion de la sala de descanso) lo decide el Blueprint de la estacion.
 */
UCLASS(ClassGroup = (Cooking), meta = (BlueprintSpawnableComponent))
class MEJIAVAZK_CAPSTONE_API UCookingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCookingComponent();

	/** Recetas conocidas. Se llenan en el Blueprint del jugador o con UnlockRecipe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	TArray<TObjectPtr<URecipeData>> KnownRecipes;

	UPROPERTY(BlueprintAssignable, Category = "Cooking|Events")
	FOnCookedSignature OnCooked;

	UPROPERTY(BlueprintAssignable, Category = "Cooking|Events")
	FOnCookFailedSignature OnCookFailed;

	UPROPERTY(BlueprintAssignable, Category = "Cooking|Events")
	FOnRecipeUnlockedSignature OnRecipeUnlocked;

	/** Aprende una receta. Devuelve false si ya la conocia. */
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	bool UnlockRecipe(URecipeData* Recipe);

	UFUNCTION(BlueprintPure, Category = "Cooking")
	bool IsRecipeKnown(const URecipeData* Recipe) const { return KnownRecipes.Contains(Recipe); }

	/** Dice si se podria cocinar ahora mismo (Success) o por que no. No gasta nada. */
	UFUNCTION(BlueprintPure, Category = "Cooking")
	ECookResult CheckCook(const URecipeData* Recipe) const;

	UFUNCTION(BlueprintPure, Category = "Cooking")
	bool CanCook(const URecipeData* Recipe) const { return CheckCook(Recipe) == ECookResult::Success; }

	/** Cocina una vez. Gasta ingredientes y entrega el resultado. */
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	ECookResult Cook(URecipeData* Recipe);

private:
	UInventoryComponent* GetInventory() const;
	bool ResultFits(const UInventoryComponent* Inventory, const URecipeData* Recipe) const;
};
