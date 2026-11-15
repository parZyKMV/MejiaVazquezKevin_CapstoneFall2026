#include "Inventory/InventoryComponent.h"

#include "Combat/HealthComponent.h"
#include "Magic/ManaComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FInventoryEntry* UInventoryComponent::FindEntry(const UItemData* Item)
{
	for (FInventoryEntry& Entry : Entries)
	{
		if (Entry.Item == Item)
		{
			return &Entry;
		}
	}
	return nullptr;
}

const FInventoryEntry* UInventoryComponent::FindEntry(const UItemData* Item) const
{
	for (const FInventoryEntry& Entry : Entries)
	{
		if (Entry.Item == Item)
		{
			return &Entry;
		}
	}
	return nullptr;
}

int32 UInventoryComponent::GetCount(const UItemData* Item) const
{
	const FInventoryEntry* Entry = Item ? FindEntry(Item) : nullptr;
	return Entry ? Entry->Count : 0;
}

TArray<FInventoryEntry> UInventoryComponent::GetEntriesByType(EItemType Type) const
{
	TArray<FInventoryEntry> Result;
	for (const FInventoryEntry& Entry : Entries)
	{
		if (Entry.Item && Entry.Item->ItemType == Type)
		{
			Result.Add(Entry);
		}
	}
	return Result;
}

int32 UInventoryComponent::GetSlotsUsed(EItemType Type) const
{
	int32 Used = 0;
	for (const FInventoryEntry& Entry : Entries)
	{
		if (Entry.Item && Entry.Item->ItemType == Type)
		{
			++Used;
		}
	}
	return Used;
}

bool UInventoryComponent::HasFreeSlot(EItemType Type) const
{
	return !UsesSlots(Type) || GetSlotsUsed(Type) < SlotsPerCategory;
}

int32 UInventoryComponent::AddItem(UItemData* Item, int32 Amount)
{
	if (!Item || Amount <= 0)
	{
		return 0;
	}

	FInventoryEntry* Entry = FindEntry(Item);

	// Objeto nuevo: necesita un espacio libre en su categoria.
	if (!Entry && !HasFreeSlot(Item->ItemType))
	{
		OnInventoryFull.Broadcast(Item);
		return 0;
	}

	const int32 Current = Entry ? Entry->Count : 0;
	const int32 Added = FMath::Clamp(Amount, 0, FMath::Max(0, MaxPerItem - Current));
	if (Added <= 0)
	{
		OnInventoryFull.Broadcast(Item);
		return 0;
	}

	if (!Entry)
	{
		Entry = &Entries.AddDefaulted_GetRef();
		Entry->Item = Item;
	}
	Entry->Count += Added;
	const int32 NewCount = Entry->Count;

	OnItemAdded.Broadcast(Item, Added);
	OnInventoryChanged.Broadcast(Item, NewCount, Added);
	return Added;
}

bool UInventoryComponent::RemoveItem(UItemData* Item, int32 Amount)
{
	if (!Item || Amount <= 0)
	{
		return false;
	}

	FInventoryEntry* Entry = FindEntry(Item);
	if (!Entry || Entry->Count < Amount)
	{
		return false;
	}

	Entry->Count -= Amount;
	const int32 NewCount = Entry->Count;

	// Al llegar a 0 se libera el espacio.
	if (NewCount <= 0)
	{
		Entries.RemoveAll([Item](const FInventoryEntry& E) { return E.Item == Item; });
	}

	OnInventoryChanged.Broadcast(Item, FMath::Max(NewCount, 0), -Amount);
	return true;
}

bool UInventoryComponent::EvaluateUse(const UItemData* Item, EItemUseFailReason& OutReason) const
{
	if (!Item || Item->ItemType != EItemType::Consumable)
	{
		OutReason = EItemUseFailReason::NotConsumable;
		return false;
	}

	if (GetCount(Item) <= 0)
	{
		OutReason = EItemUseFailReason::NoneLeft;
		return false;
	}

	const AActor* Owner = GetOwner();
	const UHealthComponent* Health = Owner ? Owner->FindComponentByClass<UHealthComponent>() : nullptr;
	const UManaComponent* Mana = Owner ? Owner->FindComponentByClass<UManaComponent>() : nullptr;

	if (Health && Health->IsDead())
	{
		OutReason = EItemUseFailReason::Dead;
		return false;
	}

	const bool bHealUseful = Item->HealAmount > 0.f && Health && Health->GetCurrentHealth() < Health->MaxHealth - KINDA_SMALL_NUMBER;
	const bool bManaUseful = Item->ManaAmount > 0.f && Mana && Mana->GetCurrentMana() < Mana->MaxMana - KINDA_SMALL_NUMBER;
	if (!bHealUseful && !bManaUseful)
	{
		OutReason = EItemUseFailReason::NoEffect;
		return false;
	}

	return true;
}

bool UInventoryComponent::CanUseItem(const UItemData* Item) const
{
	EItemUseFailReason Reason;
	return EvaluateUse(Item, Reason);
}

bool UInventoryComponent::UseItem(UItemData* Item)
{
	EItemUseFailReason Reason;
	if (!EvaluateUse(Item, Reason))
	{
		if (Item)
		{
			OnItemUseFailed.Broadcast(Item, Reason);
		}
		return false;
	}

	AActor* Owner = GetOwner();
	if (UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
	{
		if (Item->HealAmount > 0.f)
		{
			Health->Heal(Item->HealAmount);
		}
	}
	if (UManaComponent* Mana = Owner->FindComponentByClass<UManaComponent>())
	{
		if (Item->ManaAmount > 0.f)
		{
			Mana->Restore(Item->ManaAmount);
		}
	}

	RemoveItem(Item, 1);
	OnItemUsed.Broadcast(Item, GetCount(Item));
	return true;
}

void UInventoryComponent::ClearAll()
{
	Entries.Reset();
	OnInventoryChanged.Broadcast(nullptr, 0, 0);
}
