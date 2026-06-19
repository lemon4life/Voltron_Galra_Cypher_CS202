#pragma once
#include <vector>
#include <algorithm>
#include "IObserver.h"

class ISubject {
protected:
    std::vector<IObserver*> observers;

public:
    virtual ~ISubject() = default;

    void AddObserver(IObserver* observer) {
        if (std::find(observers.begin(), observers.end(), observer) == observers.end()) {
            observers.push_back(observer);
        }
    }

    void RemoveObserver(IObserver* observer) {
        observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
    }

    virtual void NotifyObservers() = 0;
};
