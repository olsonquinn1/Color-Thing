
#include "SFML\Graphics.hpp"
#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <math.h>
#include <execution>
#include <string>
#include <functional>
#include "agent.h"

#define WIDTH 800
#define WINDOW_WIDTH 1920

#define HEIGHT 450
#define WINDOW_HEIGHT 1080

using namespace std;
using namespace sf;

struct Setting {

    string name;
    float delta;
    int rule;
    float* val;
    function<void()> action;

    string shaderVar;

    Text settingText;
    Text valText;

    Setting(string name, float* val, float delta, int rule, string shaderVar, function<void()> action) : Setting(name, val, delta, rule, action) {
        this->shaderVar = shaderVar;
    }

    Setting(string name, float* val, float delta, int rule, function<void()> action) {
        this->name = name;
        this->delta = delta;
        this->rule = rule;
        this->val = val;
        this->action = action;
    }

};

struct SettingGroup {

    Text groupText;
    string name;
    vector<Setting> settings;

    SettingGroup(string name) : name(name) {}

};

string formatFloat(float f, int n) {
    float g = pow(10.0f,n);
    string s = to_string(roundf(f * g) / g);
    int delim = s.find(".");
    string front = s.substr(0, delim);
    if (n == 0) return front;
    string back = s.substr(delim + 1, s.length());
    if (back.length() > n) back = back.substr(0, back.length() - (back.length() - n));
    return front + "." + back;
}

string getSettingString(float val, int rule) {
    //formatting rule: 0 - decimal number, 1 - scaled decimal, 2 - integer number, 3 - angle , 4 - boolean
    if (rule == 4)
        return val > 0 ? "True" : "False";
    else if (rule == 3)
        return formatFloat(val * 180.0f / _Pi, 2) + (char) 248;
    else if (rule == 2)
        return to_string((int)val);
    else if (rule == 1)
        return formatFloat(val * 100.0f, 2);
    else
        return formatFloat(val, 2);
}

void updatePreview(vector<ConvexShape> &searchPreview, Vector2f previewPos, float previewScale) {
    //0: left, 1: front, 2: right
    searchPreview[0].setPointCount(3);
    searchPreview[0].setFillColor(Color(255, 0, 0, 100));
    searchPreview[0].setPoint(0, Vector2f(0, 0) + previewPos);
    searchPreview[0].setPoint(1, Vector2f(
        previewScale * cos(_Pi / -2.0f + Agent::searchAngleOffset + Agent::searchAngle * 3.0f / 2.0f),
        previewScale * sin(_Pi / -2.0f + Agent::searchAngleOffset + Agent::searchAngle * 3.0f / 2.0f)
    ) + previewPos);
    searchPreview[0].setPoint(2, Vector2f(
        previewScale * cos(_Pi / -2.0f + Agent::searchAngleOffset + Agent::searchAngle / 2.0f),
        previewScale * sin(_Pi / -2.0f + Agent::searchAngleOffset + Agent::searchAngle / 2.0f)
    ) + previewPos);

    searchPreview[1].setPointCount(3);
    searchPreview[1].setFillColor(Color(0, 255, 0, 100));
    searchPreview[1].setPoint(0, Vector2f(0, 0) + previewPos);
    searchPreview[1].setPoint(1, Vector2f(
        previewScale * cos(_Pi / -2.0f + Agent::searchAngle / 2.0f),
        previewScale * sin(_Pi / -2.0f + Agent::searchAngle / 2.0f)
    ) + previewPos);
    searchPreview[1].setPoint(2, Vector2f(
        previewScale * cos(_Pi / -2.0f - Agent::searchAngle / 2.0f),
        previewScale * sin(_Pi / -2.0f - Agent::searchAngle / 2.0f)
    ) + previewPos);

    searchPreview[2].setPointCount(3);
    searchPreview[2].setFillColor(Color(0, 0, 255, 100));
    searchPreview[2].setPoint(0, Vector2f(0, 0) + previewPos);
    searchPreview[2].setPoint(1, Vector2f(
        previewScale * cos(_Pi / -2.0f - Agent::searchAngleOffset - Agent::searchAngle / 2.0f),
        previewScale * sin(_Pi / -2.0f - Agent::searchAngleOffset - Agent::searchAngle / 2.0f)
    ) + previewPos);
    searchPreview[2].setPoint(2, Vector2f(
        previewScale * cos(_Pi / -2.0f - Agent::searchAngleOffset - Agent::searchAngle * 3.0f / 2.0f),
        previewScale * sin(_Pi / -2.0f - Agent::searchAngleOffset - Agent::searchAngle * 3.0f / 2.0f)
    ) + previewPos);
}

int main()
{

    Agent::width = WIDTH;
    Agent::height = HEIGHT;

    Vector2f scale((float)WINDOW_WIDTH / WIDTH, (float)WINDOW_HEIGHT / HEIGHT);
    Vector2f invScale(1.0f / scale.x, 1.0f / scale.y);

    float desiredFPS = 500;
    float frameTimeMS = 1000.0 / desiredFPS;
    float agentCount = 10000;

    Agent::speed = 1.0f;

    srand(time(NULL));

    RenderWindow window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Color Thing");

    RenderTexture rt;
    rt.create(WIDTH, HEIGHT);

    Texture tex;
    tex.create(WIDTH, HEIGHT);
    tex.setRepeated(true);

    Image im = tex.copyToImage();

    Sprite sp;
    sp.setTexture(rt.getTexture());
    sp.setScale(scale);

    Font font;
    if (!font.loadFromFile("Roboto-Light.ttf")) {
        cout << "font error" << endl;
        return -1;
    }

    //shader for pixel disperse and dimming effect
    float dimRate = 0.005;
    float disperseFactor = 0.3;
    Shader shader;
    shader.loadFromFile("Shader.frag", Shader::Fragment);
    shader.setUniform("texture", tex);
    shader.setUniform("dimRate", dimRate);
    shader.setUniform("disperseFactor", disperseFactor);
    shader.setUniform("imageSize", Vector2f(WIDTH, HEIGHT));

    Event event;

    Clock clock;
    Clock colorAlternateTimer;
    Clock timer;
    int frameCount = 0;

    //spawn agents
    vector<Agent> agentList;
    for (int i = 0; i < agentCount; i++) {
        agentList.push_back(Agent());
    }

    Vector2f previewPos(100, 450);
    vector<ConvexShape> searchPreview = { ConvexShape(), ConvexShape(), ConvexShape() };
    updatePreview(searchPreview, previewPos, 50);

    //Agent and environment runtime parameters
    //formatting rule: 0 - decimal number, 1 - scaled decimal, 2 - integer number, 3 - angle , 4 - boolean
    int currentSetting = 0;
    int currentGroup = 0;
    vector<SettingGroup> groups = {
        SettingGroup("Agent Options"),
        SettingGroup("Shader Options"),
        SettingGroup("Color Options"),
        SettingGroup("Oscillation Options")
    };

    //settings manipulated by GUI
    //name, val pointer, change rate, format rule, ? shader uniform name, function called on val change
    groups[0].settings = {
        Setting("Agent Count:", &agentCount, 10, 2, [&]() {
            if (agentCount < 0) agentCount == 1;
            while (agentList.size() < agentCount) {
                agentList.push_back(Agent());
            }
            while (agentList.size() > agentCount) {
                agentList.pop_back();
            }
        }),
        Setting("Speed:", &Agent::speed, 0.05f, 0, [&]() {}),
        Setting("Max Turn:", &Agent::maxTurn, _Pi / 720.0f, 3, [&]() {}),
        Setting("Bias:", &Agent::biasFactor, 0.05f, 0, [&]() {}),
        Setting("Search Size:", &Agent::searchSize, 1.0f, 0, [&]() {}),
        Setting("Search Angle:", &Agent::searchAngle, _Pi / 360.0f, 3, [&]() {
            updatePreview(searchPreview, previewPos, 50);
        }),
        Setting("Angle Offset:", &Agent::searchAngleOffset, _Pi / 360.0f, 3, [&]() {
            updatePreview(searchPreview, previewPos, 50);
        }),
        Setting("Search Model:", &Agent::searchModel, 2.0f, 2, [&]() {
            if (Agent::searchModel < 1)
                Agent::searchModel = 1;
            else if (Agent::searchModel > 2)
                Agent::searchModel = 2;
        }),
        Setting("FPS Cap:", &desiredFPS, 1, 2, [&]() {
            float frameTimeMS = 1000.0 / desiredFPS;
        })
    };
    
    groups[1].settings = {
        Setting("Dim Rate:", &dimRate, 0.0001f, 1, "dimRate", [&]() {
            shader.setUniform(
                groups[currentGroup].settings[currentSetting - 1].shaderVar,
                *groups[currentGroup].settings[currentSetting - 1].val
            );
        }),
        Setting("Disperse:", &disperseFactor, 0.005f, 0, "disperseFactor", [&]() {
            shader.setUniform(
                groups[currentGroup].settings[currentSetting - 1].shaderVar,
                *groups[currentGroup].settings[currentSetting - 1].val
            );
        })
    };

    groups[2].settings = {
        Setting("PaletteR:", &Agent::palette.x, 0.01f, 0, [&]() {
            for_each(execution::par_unseq, agentList.begin(), agentList.end(), [&](auto&& a) {
                a.updateColor();
            });
        }),
        Setting("PaletteG:", &Agent::palette.y, 0.01f, 0, [&]() {
            for_each(execution::par_unseq, agentList.begin(), agentList.end(), [&](auto&& a) {
                a.updateColor();
            });
        }),
        Setting("PaletteB:", &Agent::palette.z, 0.01f, 0, [&]() {
            for_each(execution::par_unseq, agentList.begin(), agentList.end(), [&](auto&& a) {
                a.updateColor();
            });
        })
    };

    groups[3].settings = {
        Setting("Global:", &Agent::alternate, 0.0f, 4, [&]() {
            *groups[currentGroup].settings[currentSetting - 1].val *= -1.0f;
            colorAlternateTimer.restart();
        }),
        Setting("\tR Period:", &Agent::globalPeriodR, 0.1f, 0, [&]() {}),
        Setting("\tG Period:", &Agent::globalPeriodG, 0.1f, 0, [&]() {}),
        Setting("\tB Period:", &Agent::globalPeriodB, 0.1f, 0, [&]() {}),
        Setting("Distance:", &Agent::distAlternate, 0.0f, 4, [&]() {
            *groups[currentGroup].settings[currentSetting - 1].val *= -1.0f;
            colorAlternateTimer.restart();
        }),
        Setting("\tR Distance:", &Agent::distR, 1.0f, 2, [&]() {}),
        Setting("\tG Distance:", &Agent::distG, 1.0f, 2, [&]() {}),
        Setting("\tB Distance:", &Agent::distB, 1.0f, 2, [&]() {}),
        Setting("\tR Period:", &Agent::distPeriodR, 0.1f, 0, [&]() {}),
        Setting("\tG Period:", &Agent::distPeriodG, 0.1f, 0, [&]() {}),
        Setting("\tB Period:", &Agent::distPeriodB, 0.1f, 0, [&]() {})
    };
    
    int fps = 0;
    Text fpsCounter(std::to_string(fps), font, 20);
    fpsCounter.setFillColor(Color::White);

    CircleShape mouse;
    mouse.setRadius(10);
    mouse.setOrigin(Vector2f(mouse.getRadius() / 2, mouse.getRadius() / 2));
    mouse.setFillColor(Color::White);

    RectangleShape guiBase;
    Vector2f guiPos(0, 0);
    guiBase.setFillColor(Color(50, 50, 50, 150));
    guiBase.setSize(Vector2f(240, 545));
    guiBase.setPosition(guiPos);

    RectangleShape selectedHighlight;
    selectedHighlight.setFillColor(Color(100, 100, 100, 150));
    selectedHighlight.setSize(Vector2f(240, 20));

    for (int g = 0; g < groups.size(); g++) {
        groups[g].groupText = Text(groups[g].name, font, 20);
        groups[g].groupText.setFillColor(Color::Yellow);
        groups[g].groupText.setPosition(Vector2f(guiBase.getPosition().x, guiBase.getPosition().y + 30));
        for (int s = 0; s < groups[g].settings.size(); s++) {
            groups[g].settings[s].settingText = Text(groups[g].settings[s].name, font, 20);
            groups[g].settings[s].settingText.setFillColor(Color::White);
            groups[g].settings[s].settingText.setPosition(Vector2f(guiBase.getPosition().x, guiBase.getPosition().y + 30 * (s + 2)));

            groups[g].settings[s].valText = Text(getSettingString(*groups[g].settings[s].val, groups[g].settings[s].rule), font, 20);
            groups[g].settings[s].valText.setFillColor(Color::White);
            groups[g].settings[s].valText.setPosition(Vector2f(guiBase.getPosition().x + 160, guiBase.getPosition().y + 30 * (s + 2)));
        }
    }

    selectedHighlight.setPosition(groups[0].groupText.getPosition());

    bool hideGUI = false;

    while (window.isOpen()) {

        //Event management, handle parameter changing
        //current setting = 0 indicates a setting group is selected and can be changed, >0 are the settings within that setting group
        bool settingAltered = false;
        bool settingSelected = false;
        while (window.pollEvent(event)) {
            if(event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed) {
                float multiplier = 1;
                multiplier *= Keyboard::isKeyPressed(Keyboard::LShift) ? 10 : 1;
                multiplier *= Keyboard::isKeyPressed(Keyboard::LControl) ? 10 : 1;
                if (event.key.code == Keyboard::Right) {
                    if (currentSetting == 0) {
                        currentGroup = ((currentGroup + 1) % groups.size());
                    }
                    else {
                        settingAltered = true;
                        *groups[currentGroup].settings[currentSetting - 1].val += groups[currentGroup].settings[currentSetting - 1].delta * multiplier;
                    }
                }
                if (event.key.code == Keyboard::Left) {
                    if (currentSetting == 0) {
                        currentGroup = ((currentGroup - 1) < 0 ? groups.size() - 1 : currentGroup - 1);
                    }
                    else {
                        settingAltered = true;
                        *groups[currentGroup].settings[currentSetting - 1].val -= groups[currentGroup].settings[currentSetting - 1].delta * multiplier;
                    }
                }
                if (event.key.code == Keyboard::Up) {
                    settingSelected = true;
                    currentSetting = (currentSetting - 1 < 0 ? groups[currentGroup].settings.size() : currentSetting - 1);
                }
                if (event.key.code == Keyboard::Down) {
                    settingSelected = true;
                    currentSetting = (currentSetting + 1) % (groups[currentGroup].settings.size() + 1);
                }
                if (event.key.code == Keyboard::H) {
                    hideGUI = !hideGUI;
                }
            }
        }

        if (settingAltered || settingSelected) {
            if (settingAltered && currentSetting != 0) 
                groups[currentGroup].settings[currentSetting - 1].action();

            if (settingSelected) {
                if (currentSetting == 0) 
                    selectedHighlight.setPosition(groups[currentGroup].groupText.getPosition());
                else
                    selectedHighlight.setPosition(groups[currentGroup].settings[currentSetting - 1].settingText.getPosition() + Vector2f(0, 2));
            }

            if(currentSetting != 0)
                groups[currentGroup].settings[currentSetting - 1].valText.setString(
                    getSettingString(*groups[currentGroup].settings[currentSetting - 1].val, groups[currentGroup].settings[currentSetting - 1].rule)
            );
        }

        if (clock.getElapsedTime().asMilliseconds() >= frameTimeMS) {
            clock.restart();
            
            //get a copy of current world image and feed into agents for decision making
            im = rt.getTexture().copyToImage();
            for_each(execution::par_unseq, agentList.begin(), agentList.end(), [&](auto&& a) {
                a.updatePos();
                a.alternateColor(colorAlternateTimer.getElapsedTime().asMilliseconds());
                a.updateDir(im);
            });
            tex.update(im);

            //draw world to render texture
            //note: this render texture is never cleared, thus changes made are persistent
            rt.draw(Sprite(tex), &shader);

            if (Mouse::isButtonPressed(Mouse::Left)) {
                Vector2i pixelPos = Mouse::getPosition(window);
                Vector2f worldPos = window.mapPixelToCoords(pixelPos);

                mouse.setPosition(Vector2f(
                    worldPos.x * invScale.x,
                    worldPos.y * invScale.y
                ));

                rt.draw(mouse);
            }
            
            rt.display();

            window.clear();

            window.draw(sp);

            //GUI drawing
            if (!hideGUI) {
                window.draw(guiBase);
                window.draw(selectedHighlight);
                window.draw(fpsCounter);
                window.draw(groups[currentGroup].groupText);
                for (int i = 0; i < groups[currentGroup].settings.size(); i++) {
                    window.draw(groups[currentGroup].settings[i].settingText);
                    window.draw(groups[currentGroup].settings[i].valText);
                }
                if(currentGroup == 0) for (int i = 0; i < 3; i++) {
                    window.draw(searchPreview[i]);
                }
            }
            
            window.display();

            frameCount++;
            if (timer.getElapsedTime().asMilliseconds() > 1000) {
                fpsCounter.setString("FPS: " + to_string(frameCount));
                agentList[0].debug = true;
                frameCount = 0;
                timer.restart();
            }
        }
    }
}