#pragma once
#include <vector>
#include <algorithm>
#include "IObserver.h"

// Design Pattern - Observer (Subject interface):
// TeamManager is the concrete subject and UIManager is its observer.
// Add/RemoveObserver manage subscriptions; NotifyObservers publishes snapshots
// after health, energy, roster, currency, or active-character changes.
class ISubject {
protected:
    std::vector<IObserver*> observers;

public:
    /// Releases resources owned by this ISubject instance.
    virtual ~ISubject() = default;

    /// Adds observer.
    void AddObserver(IObserver* observer) {
        if (std::find(observers.begin(), observers.end(), observer) == observers.end()) {
            observers.push_back(observer);
        }
    }

    /// Removes observer.
    void RemoveObserver(IObserver* observer) {
        observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
    }

    /// Notifies observers.
    virtual void NotifyObservers() = 0;
};
