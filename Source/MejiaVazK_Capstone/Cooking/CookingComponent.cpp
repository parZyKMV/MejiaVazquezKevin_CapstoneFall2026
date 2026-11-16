#include "Cooking/CookingComponent.h"

#include "Data/RecipeData.h"
#include "Data/ItemData.h"
#include "Inventory/InventoryComponent.h"

UCookingComponent::UCookingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UInventoryComponent* UCookingComponent::GetInventory() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

bool UCookingComponent::UnlockRecipe(URecipeData* Recipe)
{
	if (!Recipe || KnownRecipes.Contains(Recipe))
	{
		return false;
	}

	KnownRecipes.Add(Recipe);
	OnRecipeUnlocked.Broadcast(Recipe);
	return true;
}

bool UCookingComponent::ResultFits(const UInventoryComponent* Inventory, const URecipeData* Recipe) const
{
	const UItemData* Result = Recipe->Result;
	const int32 Count = FMath::Max(1, Recipe->ResultCount);
	const int32 Current = Inventory->GetCount(Result);

	if (Current + Count > Inventory->MaxPerItem)
	{
		return false;
	}

	// Ya lo tiene (o es un objeto sin espacio): no necesita un espacio nuevo.
	if (Current > 0 || Inventory->HasFreeSlot(Result->ItemType))
	{
		return true;
	}

	// Sin espacio libre: cabe si un ingrediente de su misma categoria se gasta por completo y libera uno.
	for (const FRecipeIngredient& Ingredient : Recipe->Ingredients)
	{
		if (Ingredient.Item && Ingredient.Item->ItemType == Result->ItemType
			&& Inventory->GetCount(Ingredient.Item) <= Ingredient.Count)
		{
			return true;
		}
	}
	return false;
}

ECookResult UCookingComponent::CheckCook(const URecipeData* Recipe) const
{
	if (!Recipe || !Recipe->Result || Recipe->Ingredients.Num() == 0)
	{
		return ECookResult::InvalidRecipe;
	}

	if (!KnownRecipes.Contains(Recipe))
	{
		return ECookResult::UnknownRecipe;
	}

	const UInventoryComponent* Inventory = GetInventory();
	if (!Inventory)
	{
		return ECookResult::NoInventory;
	}

	for (const FRecipeIngredient& Ingredient : Recipe->Ingredients)
	{
		if (!Ingredient.Item || !Inventory->HasItem(Ingredient.Item, Ingredient.Count))
		{
			return ECookResult::MissingIngredients;
		}
	}

	if (!ResultFits(Inventory, Recipe))
	{
		return ECookResult::NoSpace;
	}

	return ECookResult::Success;
}

ECookResult UCookingComponent::Cook(URecipeData* Recipe)
{
	const ECookResult Check = CheckCook(Recipe);
	if (Check != ECookResult::Success)
	{
		OnCookFailed.Broadcast(Recipe, Check);
		return Check;
	}

	UInventoryComponent* Inventory = GetInventory();

	for (const FRecipeIngredient& Ingredient : Recipe->Ingredients)
	{
		Inventory->RemoveItem(Ingredient.Item, Ingredient.Count);
	}

	const int32 Count = FMath::Max(1, Recipe->ResultCount);
	const int32 Added = Inventory->AddItem(Recipe->Result, Count);

	OnCooked.Broadcast(Recipe, Recipe->Result, Added);
	return ECookResult::Success;
}
