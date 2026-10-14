#include "Combat/AnimNotifyState_HitWindow.h"

#include "Combat/CombatComponent.h"

void UAnimNotifyState_HitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UCombatComponent* Combat = UCombatComponent::FindFromMesh(MeshComp))
	{
		Combat->NotifyHitWindowBegin();
	}
}

void UAnimNotifyState_HitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (UCombatComponent* Combat = UCombatComponent::FindFromMesh(MeshComp))
	{
		Combat->NotifyHitWindowTick();
	}
}

void UAnimNotifyState_HitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UCombatComponent* Combat = UCombatComponent::FindFromMesh(MeshComp))
	{
		Combat->NotifyHitWindowEnd();
	}
}

FString UAnimNotifyState_HitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Hit Window");
}