#include "RenderableEffect.h"
#include "../sequencer/Effect.h"
#include "EffectManager.h"
#include "assist/xlGridCanvasEmpty.h"

RenderableEffect::RenderableEffect(int i, std::string n,
                                   const char **data16,
                                   const char **data24,
                                   const char **data32,
                                   const char **data48,
                                   const char **data64)
    : id(i), name(n), tooltip(n), panel(nullptr), mSequenceElements(nullptr)
{
    initBitmaps(data16, data24, data32, data48, data64);
}

RenderableEffect::~RenderableEffect()
{
    //dtor
}

#ifdef __WXOSX__
double xlOSXGetMainScreenContentScaleFactor();
#endif

const wxBitmap &RenderableEffect::GetEffectIcon(int size, bool exact) const {
#ifdef __WXOSX__
    if (exact || xlOSXGetMainScreenContentScaleFactor() < 1.9) {
        if (size <= 16) {
            return icon16e;
        } else if (size <= 24) {
            return icon24e;
        } else if (size <= 32) {
            return icon32e;
        }
    }
#endif
    if (size <= 16) {
        return icon16;
    } else if (size <= 24) {
        return icon24;
    } else if (size <= 32) {
        return icon32;
    } else if (size <= 48) {
        return icon48;
    } else {
        return icon64;
    }
}


wxPanel *RenderableEffect::GetPanel(wxWindow *parent) {
    if (panel == nullptr) {
        panel = CreatePanel(parent);
    }
    return panel;
}

AssistPanel *RenderableEffect::GetAssistPanel(wxWindow *parent) {
    AssistPanel *assist_panel = new AssistPanel(parent);
    xlGridCanvas* grid = new xlGridCanvasEmpty(assist_panel->GetCanvasParent(), wxNewId(), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxFULL_REPAINT_ON_RESIZE, _T("EmptyGrid"));
    assist_panel->SetGridCanvas(grid);
    return assist_panel;
}

int RenderableEffect::DrawEffectBackground(const Effect *e, int x1, int y1, int x2, int y2) {
    if (e->HasBackgroundDisplayList()) {
        DrawGLUtils::DrawDisplayList(x1, y1, x2-x1, y2-y1, e->GetBackgroundDisplayList());
        return e->GetBackgroundDisplayList().iconSize;
    }
    return 1;
}


void AdjustAndSetBitmap(int size, wxImage &image, wxImage &dbl, wxBitmap&bitmap) {
#ifdef __WXOSX__
    if (dbl.GetHeight() == (2 * size)) {
        bitmap = wxBitmap(dbl, -1, 2.0);
    } else if (dbl.GetHeight() > (2*size)) {
        wxImage scaled = image.Scale(size*2, size*2, wxIMAGE_QUALITY_HIGH);
        bitmap = wxBitmap(scaled, -1, 2.0);
    } else
#endif
    if (image.GetHeight() == size) {
        bitmap = wxBitmap(image);
    } else {
        wxImage scaled = image.Scale(size, size, wxIMAGE_QUALITY_HIGH);
        bitmap = wxBitmap(scaled);
    }
}

void AdjustAndSetBitmap(int size, wxImage &image, wxBitmap&bitmap) {
    if (image.GetHeight() == size) {
        bitmap = wxBitmap(image);
    } else {
        wxImage scaled = image.Scale(size, size, wxIMAGE_QUALITY_HIGH);
        bitmap = wxBitmap(scaled);
    }
}

void RenderableEffect::initBitmaps(const char **data16,
                                   const char **data24,
                                   const char **data32,
                                   const char **data48,
                                   const char **data64) {
    wxImage image16(data16);
    wxImage image24(data24);
    wxImage image32(data32);
    wxImage image48(data48);
    wxImage image64(data64);
    AdjustAndSetBitmap(16, image16, image32, icon16);
    AdjustAndSetBitmap(24, image24, image48, icon24);
    AdjustAndSetBitmap(32, image32, image64, icon32);
    AdjustAndSetBitmap(48, image48, icon48);
    AdjustAndSetBitmap(64, image64, icon64);
#ifdef __WXOSX__
    AdjustAndSetBitmap(16, image16, icon16e);
    AdjustAndSetBitmap(24, image24, icon24e);
    AdjustAndSetBitmap(32, image32, icon32e);
#endif

}



bool RenderableEffect::needToAdjustSettings(const std::string &version) {
    // almost all of the settings from older 4.x series need adjustment for speed things
    return IsVersionOlder("4.2.20", version);
}


// return true if version string is older than compare string
bool RenderableEffect::IsVersionOlder(const std::string& compare, const std::string& version)
{
    wxArrayString compare_parts = wxSplit(compare, '.');
    wxArrayString version_parts = wxSplit(version, '.');
    if( wxAtoi(version_parts[0]) < wxAtoi(compare_parts[0]) ) return true;
    if( wxAtoi(version_parts[0]) > wxAtoi(compare_parts[0]) ) return false;
    if( wxAtoi(version_parts[1]) < wxAtoi(compare_parts[1]) ) return true;
    if( wxAtoi(version_parts[1]) > wxAtoi(compare_parts[1]) ) return false;
    if( wxAtoi(version_parts[2]) < wxAtoi(compare_parts[2]) ) return true;
    return false;
}
void RenderableEffect::adjustSettings(const std::string &version, Effect *effect) {
    AdjustSettingsToBeFitToTime(effect->GetEffectIndex(), effect->GetSettings(), effect->GetStartTimeMS(), effect->GetEndTimeMS(), effect->GetPalette());
}
void RenderableEffect::AdjustSettingsToBeFitToTime(int effectIdx, SettingsMap &settings, int startMS, int endMS, xlColorVector &colors)
{
    if (effectIdx == EffectManager::eff_FACES
        && settings.Get("E_CHOICE_Faces_FaceDefinition", "") == ""
        && settings.Get("E_CHOICE_Faces_TimingTrack", "") == "")
    {
        settings["E_CHOICE_Faces_FaceDefinition"] = "xlights_papagayo.xml";
    }

    int ftt = wxAtoi(settings.Get("T_CHECKBOX_FitToTime", "1"));
    switch (effectIdx)
    {
            //these effects have never used the FitToTime or speed settings, nothing to do
        case EffectManager::eff_OFF:
        case EffectManager::eff_GALAXY:
        case EffectManager::eff_FAN:
        case EffectManager::eff_MARQUEE:
        case EffectManager::eff_MORPH:
        case EffectManager::eff_SHOCKWAVE:
        case EffectManager::eff_GLEDIATOR:
        case EffectManager::eff_FACES:
            break;
        case EffectManager::eff_STROBE:
        case EffectManager::eff_TWINKLE:
            break;
            //these effects have been updated to have a dedicated repeat or speed or other control
            //and now ignore the FitToTime and Speed sliders, but the settings need adjusting
        case EffectManager::eff_ON:
            if (settings.Get("E_TEXTCTRL_On_Cycles", "") == "")
            {
                float cycles = 1.0;
                if (!ftt)
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    int totalTime = endMS - startMS;
                    int maxState = totalTime * speed / 50;
                    cycles = maxState / 200;
                }
                settings["E_TEXTCTRL_On_Cycles"] = wxString::Format("%0.2f", cycles);
            }
            break;
        case EffectManager::eff_SNOWSTORM:
            if (settings.Get("E_SLIDER_Snowstorm_Speed", "") == "")
            {
                settings["E_SLIDER_Snowstorm_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_SNOWFLAKES:
            if (settings.Get("E_SLIDER_Snowflakes_Speed", "") == "")
            {
                settings["E_SLIDER_Snowflakes_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_BUTTERFLY:
            if (settings.Get("E_SLIDER_Butterfly_Speed", "") == "")
            {
                settings["E_SLIDER_Butterfly_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_CIRCLES:
            if (settings.Get("E_SLIDER_Circles_Speed", "") == "")
            {
                settings["E_SLIDER_Circles_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_LIFE:
            if (settings.Get("E_SLIDER_Life_Speed", "") == "")
            {
                settings["E_SLIDER_Life_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_METEORS:
            if (settings.Get("E_SLIDER_Meteors_Speed", "") == "")
            {
                settings["E_SLIDER_Meteors_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_TREE:
            if (settings.Get("E_SLIDER_Tree_Speed", "") == "")
            {
                settings["E_SLIDER_Tree_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_PINWHEEL:
            if (settings.Get("E_TEXTCTRL_Pinwheel_Speed", "") == "")
            {
                settings["E_TEXTCTRL_Pinwheel_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_PLASMA:
            if (settings.Get("E_TEXTCTRL_Plasma_Speed", "") == "")
            {
                settings["E_TEXTCTRL_Plasma_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_TEXT:
            if (settings.Get("E_TEXTCTRL_Text_Speed1", "") == "")
            {
                settings["E_TEXTCTRL_Text_Speed1"] = settings.Get("T_SLIDER_Speed", "10");
            }
            if (settings.Get("E_TEXTCTRL_Text_Speed2", "") == "")
            {
                settings["E_TEXTCTRL_Text_Speed2"] = settings.Get("T_SLIDER_Speed", "10");
            }
            if (settings.Get("E_TEXTCTRL_Text_Speed3", "") == "")
            {
                settings["E_TEXTCTRL_Text_Speed3"] = settings.Get("T_SLIDER_Speed", "10");
            }
            if (settings.Get("E_TEXTCTRL_Text_Speed4", "") == "")
            {
                settings["E_TEXTCTRL_Text_Speed4"] = settings.Get("T_SLIDER_Speed", "10");
            }

            if (settings.Get("E_SLIDER_Text_Position1", "") != "") {
                int pos = wxAtoi(settings.Get("E_SLIDER_Text_Position1", "50")) * 2 - 100;
                settings.erase("E_SLIDER_Text_Position1");
                settings["E_SLIDER_Text_YStart1"] = wxString::Format("%d", pos);
                settings["E_SLIDER_Text_XStart1"] = wxString::Format("%d", pos);
                settings["E_SLIDER_Text_XEnd1"] = wxString::Format("%d", pos);
                settings["E_SLIDER_Text_YEnd1"] = wxString::Format("%d", pos);
            }
            break;
        case EffectManager::eff_WAVE:
            if (settings.Get("E_TEXTCTRL_Wave_Speed", "") == "")
            {
                settings["E_TEXTCTRL_Wave_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            break;
        case EffectManager::eff_SPIROGRAPH:
            if (settings.Get("E_TEXTCTRL_Spirograph_Speed", "") == "")
            {
                settings["E_TEXTCTRL_Spirograph_Speed"] = settings.Get("T_SLIDER_Speed", "10");
            }
            if (settings.Get("E_CHECKBOX_Spirograph_Animate", "") != "")
            {
                int i = wxAtoi(settings.Get("E_CHECKBOX_Spirograph_Animate", "0"));
                settings["E_TEXTCTRL_Spirograph_Animate"] = (i == 0 ? "0" : "10");
                settings.erase("E_CHECKBOX_Spirograph_Animate");
            }
            break;

        case EffectManager::eff_COLORWASH:
            if (settings.Get("E_TEXTCTRL_ColorWash_Cycles", "") == "")
            {
                double count = wxAtoi(settings.Get("E_SLIDER_ColorWash_Count", "1"));
                settings.erase("E_SLIDER_ColorWash_Count");
                if (settings["T_CHECKBOX_FitToTime"] == "1")
                {
                    count = 1.0;
                    settings["E_CHECKBOX_ColorWash_CircularPalette"] = "0";
                }
                else
                {
                    settings["E_CHECKBOX_ColorWash_CircularPalette"] = "1";
                }
                settings["E_TEXTCTRL_ColorWash_Cycles"] = wxString::Format("%0.2f", count);
            }
            break;
        case EffectManager::eff_FIRE:
            if (settings.Get("E_TEXTCTRL_Fire_GrowthCycles", "") == "")
            {
                bool grow = settings["E_CHECKBOX_Fire_GrowFire"] == "1";
                settings.erase("E_CHECKBOX_Fire_GrowFire");
                if (grow)
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    int totalTime = endMS - startMS;
                    double maxState = totalTime * speed / 50;
                    double cycles = maxState / 500.0;
                    settings["E_TEXTCTRL_Fire_GrowthCycles"] = wxString::Format("%0.2f", cycles);
                }
                else
                {
                    settings["E_TEXTCTRL_Fire_GrowthCycles"] = "0.0";
                }
            }
            break;
        case EffectManager::eff_FIREWORKS:
            if (settings.Get("E_SLIDER_Fireworks_Number_Explosions", "") != "")
            {
                int cnt = wxAtoi(settings.Get("E_SLIDER_Fireworks_Number_Explosions", "10"));
                settings.erase("E_SLIDER_Fireworks_Number_Explosions");
                int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                int total = (speed * cnt) / 50;
                if (total > 50)
                {
                    total = 50;
                }
                if (total < 1)
                {
                    total = 1;
                }
                settings["E_SLIDER_Fireworks_Explosions"] = wxString::Format("%d", total);
            }
            break;
        case EffectManager::eff_RIPPLE:
            if (settings.Get("E_TEXTCTRL_Ripple_Cycles", "") == "")
            {
                float cycles = 1.0;
                if (!ftt)
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    int totalTime = endMS - startMS;
                    int maxState = totalTime * speed / 50;
                    cycles = maxState / 200;
                }
                settings["E_TEXTCTRL_Ripple_Cycles"] = wxString::Format("%0.2f", cycles);
            }
            break;
        case EffectManager::eff_BARS:
            if (settings.Get("E_TEXTCTRL_Bars_Cycles", "") == "")
            {
                float cycles = 1.0;
                wxString dir = settings["E_CHOICE_Bars_Direction"];
                if (!ftt)
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    int totalTime = endMS - startMS;
                    int maxState = totalTime * speed / 50;
                    if (dir.Contains("Altern"))
                    {
                        cycles = maxState * 2;
                    }
                    else
                    {
                        cycles = maxState / 200;
                    }
                }
                settings["E_TEXTCTRL_Bars_Cycles"] = wxString::Format("%0.2f", cycles);
            }
            break;
        case EffectManager::eff_SPIRALS:
            if (settings.Get("E_TEXTCTRL_Spirals_Movement", "") == "")
            {
                float cycles = 1.0;
                int dir = wxAtoi(settings.Get("E_SLIDER_Spirals_Direction", "1"));
                settings.erase("E_SLIDER_Spirals_Direction");
                if (!ftt)
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    int totalTime = endMS - startMS;
                    int maxState = totalTime * speed / 50;
                    cycles = maxState / 600;
                }
                settings["E_TEXTCTRL_Spirals_Movement"] = wxString::Format("%0.2f", dir * cycles);
            }
            break;
        case EffectManager::eff_CURTAIN:
            if (settings.Get("E_TEXTCTRL_Curtain_Speed", "") == "")
            {
                float cycles = 1.0;
                if (!ftt)
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    int totalTime = endMS - startMS;
                    int maxState = totalTime * speed / 50;
                    cycles = maxState / 200;
                }
                settings["E_TEXTCTRL_Curtain_Speed"] = wxString::Format("%0.2f", cycles);
            }
            break;
        case EffectManager::eff_SINGLESTRAND:
            if ("Skips" == settings["E_NOTEBOOK_SSEFFECT_TYPE"])
            {
                if (settings.Get("E_SLIDER_Skips_Advance", "") == "")
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    settings["E_SLIDER_Skips_Advance"] = wxString::Format("%d", speed - 1);
                }
            }
            else
            {
                wxString type = settings.Get("E_CHOICE_Chase_Type1", "Left-Right");
                if (type == "Auto reverse")
                {
                    type = "Bounce from Left";
                    settings["E_CHOICE_Chase_Type1"] = type;
                }
                else if (type == "Bounce" || type == "Pacman")
                {
                    type = "Dual Bounce";
                    settings["E_CHOICE_Chase_Type1"] = type;
                }
                if (settings.Get("E_TEXTCTRL_Chase_Rotations", "") == "")
                {
                    float cycles = 1.0;
                    if (!ftt)
                    {
                        int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                        int totalTime = endMS - startMS;
                        int maxState = totalTime * speed / 50;
                        cycles = maxState / 250.0;
                    }
                    settings["E_TEXTCTRL_Chase_Rotations"] = wxString::Format("%0.2f", cycles);
                }
            }
            break;
        case EffectManager::eff_SHIMMER:
            if (settings.Get("E_TEXTCTRL_Shimmer_Cycles", "") == "")
            {
                float cycles = 1.0;
                int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                int totalTime = endMS - startMS;
                int maxState = totalTime * speed / 50;
                cycles = maxState / (100.0 * colors.size());
                settings["E_TEXTCTRL_Shimmer_Cycles"] = wxString::Format("%0.2f", cycles);
            }
            break;
        case EffectManager::eff_PICTURES:
            if (settings.Get("E_TEXTCTRL_Pictures_FrameRateAdj", "") == "")
            {
                if (settings.Get("E_CHECKBOX_MovieIs20FPS", "") == "1")
                {
                    settings["E_TEXTCTRL_Pictures_FrameRateAdj"] = "1.0";
                }
                else if (settings.Get("E_SLIDER_Pictures_GifSpeed", "") == "0")
                {
                    settings["E_TEXTCTRL_Pictures_FrameRateAdj"] = "0.0";
                }
                else if (!ftt)
                {
                    int speed = wxAtoi(settings.Get("T_SLIDER_Speed", "10"));
                    int totalTime = endMS - startMS;
                    int maxState = totalTime * speed / 50;
                    double cycles = maxState / 300.0;
                    settings["E_TEXTCTRL_Pictures_Speed"] = wxString::Format("%0.2f", cycles);
                }

                settings.erase("E_CHECKBOX_MovieIs20FPS");
                settings.erase("E_SLIDER_Pictures_GifSpeed");
            }
            break;
        case EffectManager::eff_GARLANDS:
            //Don't attempt to map the Garlands speed settings.  In v3, the Garland speed depended on the Speed setting, the
            //Spacing setting as well as the height of the model.  We don't have the height of the model here so really
            //no way to figure out the speed or an appropriate mapping
            break;

            //these all need code updated and new sliders and such before we can map them
            //these all have state/speed requirements
        case EffectManager::eff_PIANO:
            break;
    }
    settings.erase("T_CHECKBOX_FitToTime");
    settings.erase("T_SLIDER_Speed");
}

