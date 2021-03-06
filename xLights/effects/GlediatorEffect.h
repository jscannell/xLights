#ifndef GLEDIATOREFFECT_H
#define GLEDIATOREFFECT_H

#include "RenderableEffect.h"


class GlediatorEffect : public RenderableEffect
{
    public:
        GlediatorEffect(int id);
        virtual ~GlediatorEffect();
        virtual bool CanBeRandom() override {return false;}
        virtual void SetSequenceElements(SequenceElements *els) override;
        virtual void SetDefaultParameters(Model *cls) override;
        virtual void Render(Effect *effect, const SettingsMap &settings, RenderBuffer &buffer) override;
        virtual std::list<std::string> CheckEffectSettings(const SettingsMap& settings, AudioManager* media, Model* model, Effect* eff) override;
        virtual void adjustSettings(const std::string &version, Effect *effect) override;
protected:
        virtual wxPanel *CreatePanel(wxWindow *parent) override;
    private:
};

#endif // GLEDIATOREFFECT_H
