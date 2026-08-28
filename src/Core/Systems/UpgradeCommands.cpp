#include "Core/Systems/UpgradeCommands.h"
#include "Entities/Player/Paladin.h"

// --- UpgradeHealthCommand ---
bool UpgradeHealthCommand::Execute(Paladin* paladin) {
    return paladin ? paladin->UpgradeStat(StatType::Health) : false;
}

bool UpgradeHealthCommand::CanExecute(const Paladin* paladin) const {
    return paladin ? paladin->CanUpgradeStat(StatType::Health) : false;
}

float UpgradeHealthCommand::GetProgress(const Paladin* paladin) const {
    return paladin ? paladin->GetStatProgress(StatType::Health) : 0.0f;
}

// --- UpgradeAttackSpeedCommand ---
bool UpgradeAttackSpeedCommand::Execute(Paladin* paladin) {
    return paladin ? paladin->UpgradeStat(StatType::AttackSpeed) : false;
}

bool UpgradeAttackSpeedCommand::CanExecute(const Paladin* paladin) const {
    return paladin ? paladin->CanUpgradeStat(StatType::AttackSpeed) : false;
}

float UpgradeAttackSpeedCommand::GetProgress(const Paladin* paladin) const {
    return paladin ? paladin->GetStatProgress(StatType::AttackSpeed) : 0.0f;
}

// --- UpgradeSpeedCommand ---
bool UpgradeSpeedCommand::Execute(Paladin* paladin) {
    return paladin ? paladin->UpgradeStat(StatType::Speed) : false;
}

bool UpgradeSpeedCommand::CanExecute(const Paladin* paladin) const {
    return paladin ? paladin->CanUpgradeStat(StatType::Speed) : false;
}

float UpgradeSpeedCommand::GetProgress(const Paladin* paladin) const {
    return paladin ? paladin->GetStatProgress(StatType::Speed) : 0.0f;
}

// --- UpgradeDamageCommand ---
bool UpgradeDamageCommand::Execute(Paladin* paladin) {
    return paladin ? paladin->UpgradeStat(StatType::Damage) : false;
}

bool UpgradeDamageCommand::CanExecute(const Paladin* paladin) const {
    return paladin ? paladin->CanUpgradeStat(StatType::Damage) : false;
}

float UpgradeDamageCommand::GetProgress(const Paladin* paladin) const {
    return paladin ? paladin->GetStatProgress(StatType::Damage) : 0.0f;
}

// --- UpgradeManager ---
bool UpgradeManager::ExecuteUpgrade(Paladin* paladin, StatType stat) {
    if (!paladin) return false;
    return paladin->UpgradeStat(stat);
}

bool UpgradeManager::CanUpgrade(const Paladin* paladin, StatType stat) {
    if (!paladin) return false;
    return paladin->CanUpgradeStat(stat);
}

float UpgradeManager::GetStatProgress(const Paladin* paladin, StatType stat) {
    if (!paladin) return 0.0f;
    return paladin->GetStatProgress(stat);
}
