#pragma once

class Paladin;

enum class StatType {
    Health,
    AttackSpeed,
    Speed,
    Damage
};

class IUpgradeCommand {
public:
    virtual ~IUpgradeCommand() = default;
    virtual bool Execute(Paladin* paladin) = 0;
    virtual bool CanExecute(const Paladin* paladin) const = 0;
    virtual float GetProgress(const Paladin* paladin) const = 0;
    virtual StatType GetStatType() const = 0;
};

class UpgradeHealthCommand : public IUpgradeCommand {
public:
    bool Execute(Paladin* paladin) override;
    bool CanExecute(const Paladin* paladin) const override;
    float GetProgress(const Paladin* paladin) const override;
    StatType GetStatType() const override { return StatType::Health; }
};

class UpgradeAttackSpeedCommand : public IUpgradeCommand {
public:
    bool Execute(Paladin* paladin) override;
    bool CanExecute(const Paladin* paladin) const override;
    float GetProgress(const Paladin* paladin) const override;
    StatType GetStatType() const override { return StatType::AttackSpeed; }
};

class UpgradeSpeedCommand : public IUpgradeCommand {
public:
    bool Execute(Paladin* paladin) override;
    bool CanExecute(const Paladin* paladin) const override;
    float GetProgress(const Paladin* paladin) const override;
    StatType GetStatType() const override { return StatType::Speed; }
};

class UpgradeDamageCommand : public IUpgradeCommand {
public:
    bool Execute(Paladin* paladin) override;
    bool CanExecute(const Paladin* paladin) const override;
    float GetProgress(const Paladin* paladin) const override;
    StatType GetStatType() const override { return StatType::Damage; }
};

class UpgradeManager {
public:
    static bool ExecuteUpgrade(Paladin* paladin, StatType stat);
    static bool CanUpgrade(const Paladin* paladin, StatType stat);
    static float GetStatProgress(const Paladin* paladin, StatType stat);
};
