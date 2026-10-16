#pragma once
#include "modules/module.hpp"

class Reach : public Module {
public:
    Reach() : Module("Reach", "Allonge la portée d'attaque (max 6 blocs : limite du serveur intégré).") {}

    void onUpdate(Ctx& c) override;
    void drawSettings() override;

    // Attaque l'entité visée si elle est au-delà de la portée vanilla mais dans la portée du module.
    // Retourne true si une attaque a été envoyée.
    bool tryAttack(Ctx& c, bool swing);

private:
    jobject findTarget(Ctx& c, double& dist); // ref locale ou nullptr

    float range_ = 4.0f;
    bool wasDown_ = false;
};
