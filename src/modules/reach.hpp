#pragma once
#include "modules/module.hpp"

class Reach : public Module {
public:
    Reach() : Module("Reach", "Portée étendue (3 à 6 blocs). Le serveur peut refuser les attaques hors portée.") {}

    void onTick(Ctx& c) override;
    void reset() { wasDown_ = false; }
    void drawSettings() override;

    // Attaque l'entité visée si elle est au-delà de la portée vanilla mais dans la portée du module.
    // Retourne true si une attaque a été envoyée.
    bool tryAttack(Ctx& c, bool swing);

private:
    jobject findTarget(Ctx& c, double& dist); // ref locale ou nullptr

    float range_ = 4.0f;
    bool wasDown_ = false;
};
