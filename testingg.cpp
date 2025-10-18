// testing.cpp
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <cmath>
#include <vector>

using namespace std;
using namespace std::chrono;

enum State { MENU, WORK, BREAK, FINISHED };

int main() {
    // Basic window
    unsigned int winW = 400;
    unsigned int winH = 560;
    sf::RenderWindow window(sf::VideoMode(winW, winH), "Owl Pomodoro 🦉");
    window.setFramerateLimit(60);

    // Font
    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        cerr << "Font not found\n";
        return -1;
    }

    // Owl image
    sf::Texture owlTexture;
    if (!owlTexture.loadFromFile("owl.png")) {
        cerr << "owl.png not found\n";
        return -1;
    }

    // Music (lofi) as before
    sf::Music lofiMusic;
    if (!lofiMusic.openFromFile("tick.wav")) {
        cerr << "tick.wav not found\n";
        return -1;
    }
    lofiMusic.setLoop(true);
    lofiMusic.setVolume(45.f);

    // Beep
    sf::SoundBuffer beepBuf;
    if (!beepBuf.loadFromFile("beep.wav")) {
        cerr << "beep.wav not found\n";
        return -1;
    }
    sf::Sound beepSound(beepBuf);
    beepSound.setVolume(100.f);

    // Colors
    sf::Color darkRed(40, 0, 0);
    sf::Color buttonColor(200, 0, 0);
    sf::Color textWhite = sf::Color::White;

    // Text objects (we'll center with origins)
    sf::Text titleText("Pomodoro", font, 30);
    titleText.setFillColor(buttonColor);

    sf::Text timerText("25:00", font, 52);
    timerText.setFillColor(textWhite);

    sf::Text finalMessage("YOU DID GREAT 🦉", font, 28);
    finalMessage.setFillColor(sf::Color::Yellow);

    // Start/Pause button (rounded look by drawing a rectangle with outline)
sf::RectangleShape startButton(sf::Vector2f(160, 52));
startButton.setFillColor(buttonColor);
startButton.setOutlineColor(sf::Color(120, 0, 0));
startButton.setOutlineThickness(2.f);

sf::Text startText("START", font, 22);
startText.setFillColor(textWhite);

    // Settings circular button (top-left): draw circle + gear text
    float settingsR = 20.f;
    sf::CircleShape settingsCircle(settingsR);
    settingsCircle.setFillColor(sf::Color(120, 0, 0));
    settingsCircle.setOutlineColor(sf::Color(180, 0, 0));
    settingsCircle.setOutlineThickness(2.f);
    settingsCircle.setPosition(12.f, 12.f);

    sf::Text gearText("⚙", font, 20);
    gearText.setFillColor(textWhite);
    gearText.setStyle(sf::Text::Bold);

    // Settings panel overlay
    bool showSettings = false;
    sf::RectangleShape panel(sf::Vector2f(360, 170));
    panel.setFillColor(sf::Color(20, 0, 0, 240));
    panel.setPosition(20.f, 80.f);
    panel.setOutlineColor(sf::Color(80, 0, 0));
    panel.setOutlineThickness(2.f);

    // Settings values
    int workMinutes = 25;
    int breakMinutes = 5;
    bool bigScreenEnabled = true; // default on

    sf::Text workLabel("Work (min):", font, 16);
    workLabel.setFillColor(textWhite);
    sf::Text breakLabel("Break (min):", font, 16);
    breakLabel.setFillColor(textWhite);

    sf::Text workValue(to_string(workMinutes), font, 18);
    workValue.setFillColor(textWhite);
    sf::Text breakValue(to_string(breakMinutes), font, 18);
    breakValue.setFillColor(textWhite);

    // +/- buttons for settings
    sf::RectangleShape plusW(sf::Vector2f(30, 28)); plusW.setFillColor(buttonColor);
    sf::RectangleShape minusW(sf::Vector2f(30, 28)); minusW.setFillColor(buttonColor);
    sf::RectangleShape plusB(sf::Vector2f(30, 28)); plusB.setFillColor(buttonColor);
    sf::RectangleShape minusB(sf::Vector2f(30, 28)); minusB.setFillColor(buttonColor);
    sf::Text plusText("+", font, 18); plusText.setFillColor(textWhite);
    sf::Text minusText("-", font, 22); minusText.setFillColor(textWhite);

    // Big screen toggle visual
    sf::Text toggleLabel("Big Screen Mode:", font, 16);
    toggleLabel.setFillColor(textWhite);
    sf::RectangleShape toggleBox(sf::Vector2f(40, 22));
    toggleBox.setFillColor(bigScreenEnabled ? sf::Color(200,0,0) : sf::Color(80,0,0));
    sf::Text toggleOnOff(bigScreenEnabled ? "ON" : "OFF", font, 12);
    toggleOnOff.setFillColor(textWhite);

    // Panel Start button
    sf::RectangleShape panelStart(sf::Vector2f(140, 36));
    panelStart.setFillColor(buttonColor);
    sf::Text panelStartText("Start Session", font, 16);
    panelStartText.setFillColor(textWhite);

    // Progress bar geometry (bottom)
    const float barLeft = 40.f;
    const float barRight = 360.f;
    const float barY = 500.f;
    const float barHeight = 18.f;
    const int totalCheckpoints = 4;
    vector<float> checkpointX(totalCheckpoints);
    for (int i = 0; i < totalCheckpoints; ++i) {
        checkpointX[i] = barLeft + (barRight - barLeft) * (i / float(totalCheckpoints - 1));
    }

    // Small checkpoint owl
    sf::Sprite checkpointOwl(owlTexture);
    checkpointOwl.setScale(0.12f, 0.12f);

    // Moving owl (on progress bar)
    sf::Sprite movingOwl(owlTexture);
    movingOwl.setScale(0.22f, 0.22f);
    float owlX = checkpointX[0];
    float owlY = barY - 40.f;
    movingOwl.setPosition(owlX - 24.f, owlY);

    // Big centered owl (for big screen mode) - scale will change
    sf::Sprite bigOwl(owlTexture);

    // State & timer variables
    State state = MENU;
    bool running = false;
    int totalSeconds = workMinutes * 60;
    auto startTime = steady_clock::now();
    int completedSessions = 0;

    // Animation variables
    bool animating = false;
    float animStartX = owlX;
    float animTargetX = owlX;
    float animDuration = 1.0f;
    auto animStartTime = steady_clock::now();

    // Big mode active flag
    bool bigModeActive = false;

    // helper for formatting time
    auto secondsToStr = [](int s) {
        int m = s / 60;
        int sec = s % 60;
        string mm = (m < 10 ? "0" + to_string(m) : to_string(m));
        string ss = (sec < 10 ? "0" + to_string(sec) : to_string(sec));
        return mm + ":" + ss;
    };

    // Pre-calc positions (will recalc centering per frame)
    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
            if (ev.type == sf::Event::Resized) {
                // keep view consistent
                sf::FloatRect visibleArea(0, 0, ev.size.width, ev.size.height);
                window.setView(sf::View(visibleArea));
                winW = ev.size.width; winH = ev.size.height;
            }

            if (ev.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mpos = sf::Mouse::getPosition(window);

                // settings circle toggle
                if (settingsCircle.getGlobalBounds().contains(mpos.x, mpos.y)) {
                    showSettings = !showSettings;
                }

                if (showSettings) {
                    // plus/minus work (positions relative to panel)
                    sf::FloatRect pbox = panel.getGlobalBounds();
                    float px = pbox.left, py = pbox.top;
                    if (plusW.getGlobalBounds().contains(mpos.x, mpos.y)) {
                        if (workMinutes < 180) workMinutes++;
                        workValue.setString(to_string(workMinutes));
                    } else if (minusW.getGlobalBounds().contains(mpos.x, mpos.y)) {
                        if (workMinutes > 1) workMinutes--;
                        workValue.setString(to_string(workMinutes));
                    }
                    if (plusB.getGlobalBounds().contains(mpos.x, mpos.y)) {
                        if (breakMinutes < 60) breakMinutes++;
                        breakValue.setString(to_string(breakMinutes));
                    } else if (minusB.getGlobalBounds().contains(mpos.x, mpos.y)) {
                        if (breakMinutes > 1) breakMinutes--;
                        breakValue.setString(to_string(breakMinutes));
                    }
                    // toggle big screen
                    if (toggleBox.getGlobalBounds().contains(mpos.x, mpos.y)) {
                        bigScreenEnabled = !bigScreenEnabled;
                        toggleBox.setFillColor(bigScreenEnabled ? sf::Color(200,0,0) : sf::Color(80,0,0));
                        toggleOnOff.setString(bigScreenEnabled ? "ON" : "OFF");
                    }
                    // Start from panel
                    if (panelStart.getGlobalBounds().contains(mpos.x, mpos.y)) {
                        totalSeconds = workMinutes * 60;
                        completedSessions = 0;
                        state = WORK;
                        running = true;
                        startTime = steady_clock::now();
                        if (lofiMusic.getStatus() != sf::Music::Playing) lofiMusic.play();
                        showSettings = false;
                        // big mode
                        if (bigScreenEnabled) {
                            bigModeActive = true;
                        } else {
                            bigModeActive = false;
                        }
                    }
                } else {
                    // If bigModeActive, check for exit small button top-right
                    if (bigModeActive) {
                        // small close circle
                        sf::CircleShape closeCircle(16.f);
                        closeCircle.setPosition(winW - 40.f, 8.f);
                        if (closeCircle.getGlobalBounds().contains(mpos.x, mpos.y)) {
                            bigModeActive = false;
                        }
                    } else {
                        // Start/pause button when not in settings and not bigMode
                        if (startButton.getGlobalBounds().contains(mpos.x, mpos.y)) {
                            if (state == MENU) {
                                // start with settings values
                                totalSeconds = workMinutes * 60;
                                completedSessions = 0;
                                state = WORK;
                                running = true;
                                startTime = steady_clock::now();
                                if (lofiMusic.getStatus() != sf::Music::Playing) lofiMusic.play();
                            } else if (state != FINISHED) {
                                running = !running;
                                if (running) {
                                    startTime = steady_clock::now();
                                    if (state == WORK && lofiMusic.getStatus() != sf::Music::Playing) lofiMusic.play();
                                } else {
                                    lofiMusic.pause();
                                }
                            } else { // FINISHED, pressing START resets
                                totalSeconds = workMinutes * 60;
                                completedSessions = 0;
                                state = WORK;
                                running = true;
                                startTime = steady_clock::now();
                                if (lofiMusic.getStatus() != sf::Music::Playing) lofiMusic.play();
                            }
                        }
                    }
                }
            }
        }

        // Timer updates
        if (running && state != FINISHED) {
            auto now = steady_clock::now();
            auto elapsed = duration_cast<seconds>(now - startTime).count();
            if (elapsed >= 1) {
                totalSeconds -= (int)elapsed;
                startTime = now;
            }

            if (totalSeconds <= 0) {
                // session ended
                beepSound.play();
                if (state == WORK) {
                    completedSessions++;
                    // animate owl movement to next checkpoint
                    animating = true;
                    animStartX = owlX;
                    int nextIndex = min(completedSessions, totalCheckpoints - 1);
                    animTargetX = checkpointX[nextIndex];
                    animStartTime = steady_clock::now();
                    // stop music for break
                    lofiMusic.stop();

                    if (completedSessions >= totalCheckpoints) {
                        state = FINISHED;
                        running = false;
                    } else {
                        state = BREAK;
                        totalSeconds = breakMinutes * 60;
                    }
                } else if (state == BREAK) {
                    state = WORK;
                    totalSeconds = workMinutes * 60;
                    // resume music
                    if (lofiMusic.getStatus() != sf::Music::Playing) lofiMusic.play();
                }
            }
        }

        // Animate moving owl
        if (animating) {
            auto now = steady_clock::now();
            float t = duration_cast<milliseconds>(now - animStartTime).count() / 1000.0f;
            float u = min(1.0f, t / animDuration);
            float s = u * u * (3 - 2 * u); // smoothstep
            float curX = animStartX + (animTargetX - animStartX) * s;
            owlX = curX;
            movingOwl.setPosition(owlX - 24.f, owlY);
            if (u >= 1.0f) animating = false;
        }

        // Update displayed timer string
        int displaySeconds = max(0, totalSeconds);
        string timeStr = secondsToStr(displaySeconds);
        timerText.setString(timeStr);

        // Layout & centering each frame
        float cx = winW / 2.0f;
        // center title
        sf::FloatRect tb = titleText.getLocalBounds();
        titleText.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        titleText.setPosition(cx, 210.f);

        // center timer
        sf::FloatRect tb2 = timerText.getLocalBounds();
        timerText.setOrigin(tb2.left + tb2.width / 2.f, tb2.top + tb2.height / 2.f);
        timerText.setPosition(cx, 280.f);

        // center start button and text
        startButton.setSize(sf::Vector2f(160, 52));
        startButton.setOrigin(startButton.getSize().x / 2.f, startButton.getSize().y / 2.f);
        startButton.setPosition(cx, 360.f);
        startText.setCharacterSize(22);
        sf::FloatRect stb = startText.getLocalBounds();
        startText.setOrigin(stb.left + stb.width / 2.f, stb.top + stb.height / 2.f);
        startText.setPosition(cx, 360.f - 4.f);

        // settings gear position
        settingsCircle.setPosition(12.f, 12.f);
        gearText.setPosition(12.f + settingsR - 12.f, 12.f - 2.f);

        // panel items positions
        panel.setPosition(20.f, 80.f);
        workLabel.setPosition(40.f, 100.f);
        workValue.setPosition(200.f, 98.f);
        breakLabel.setPosition(40.f, 136.f);
        breakValue.setPosition(200.f, 134.f);
        plusW.setPosition(240.f, 96.f); minusW.setPosition(280.f, 96.f);
        plusB.setPosition(240.f, 134.f); minusB.setPosition(280.f, 134.f);
        plusText.setPosition(246.f, 98.f); minusText.setPosition(286.f, 96.f);

        panelStart.setPosition(130.f, 184.f);
        panelStartText.setPosition(150.f, 188.f);

        // toggle position
        toggleLabel.setPosition(40.f, 165.f);
        toggleBox.setPosition(200.f, 162.f);
        toggleOnOff.setPosition(206.f, 165.f);

        // Update toggle color label text
        toggleBox.setFillColor(bigScreenEnabled ? sf::Color(200,0,0) : sf::Color(80,0,0));
        toggleOnOff.setString(bigScreenEnabled ? "ON" : "OFF");

        // progress bar stripes positions unchanged (barLeft, barRight, barY)
        // update checkpoint positions if window resized
        for (int i = 0; i < totalCheckpoints; ++i) {
            checkpointX[i] = barLeft + (barRight - barLeft) * (i / float(totalCheckpoints - 1));
        }

        // Draw
        window.clear(darkRed);

        // settings circle + gear
        window.draw(settingsCircle);
        window.draw(gearText);

        // If in bigModeActive (focused view)
        if (bigModeActive) {
            // big owl centered and large
            float bigScale = min(winW, winH) / 700.f * 1.8f; // scale factor
            bigOwl.setTexture(owlTexture);
            bigOwl.setScale(bigScale, bigScale);
            sf::FloatRect ob = bigOwl.getLocalBounds();
            bigOwl.setOrigin(ob.left + ob.width/2.f, ob.top + ob.height/2.f);
            bigOwl.setPosition(winW/2.f, winH/2.f - 40.f);

            // big timer large
            sf::Text bigTimer(timerText);
            bigTimer.setCharacterSize(110);
            sf::FloatRect bt = bigTimer.getLocalBounds();
            bigTimer.setOrigin(bt.left + bt.width/2.f, bt.top + bt.height/2.f);
            bigTimer.setPosition(winW/2.f, winH/2.f + 120.f);

            window.draw(bigOwl);
            window.draw(bigTimer);

            // progress bar at bottom (draw gradient)
            const int stripes = 80;
            for (int i = 0; i < stripes; ++i) {
                float x0 = barLeft + (barRight - barLeft) * (i / float(stripes));
                float x1 = barLeft + (barRight - barLeft) * ((i + 1) / float(stripes));
                float t = i / float(stripes - 1);
                sf::Uint8 r = (sf::Uint8)((1 - t) * 200 + t * 120);
                sf::Uint8 g = (sf::Uint8)((1 - t) * 0 + t * 0);
                sf::Uint8 b = (sf::Uint8)((1 - t) * 0 + t * 160);
                sf::RectangleShape stripe(sf::Vector2f(x1 - x0, barHeight));
                stripe.setPosition(x0, barY);
                stripe.setFillColor(sf::Color(r,g,b));
                window.draw(stripe);
            }
            // checkpoints
            for (int i = 0; i < totalCheckpoints; ++i) {
                float cx = checkpointX[i];
                checkpointOwl.setPosition(cx - 10.f, barY - 10.f);
                if (i < (totalCheckpoints - completedSessions)) window.draw(checkpointOwl);
                else {
                    sf::Sprite dim = checkpointOwl;
                    dim.setColor(sf::Color(80,80,80,160));
                    dim.setPosition(cx - 10.f, barY - 10.f);
                    window.draw(dim);
                }
            }
            // moving owl
            window.draw(movingOwl);

            // small close circle top-right to exit big mode
            sf::CircleShape closeCircle(16.f);
            closeCircle.setFillColor(sf::Color(120,0,0));
            closeCircle.setOutlineColor(sf::Color(180,0,0));
            closeCircle.setOutlineThickness(2.f);
            closeCircle.setPosition(winW - 40.f, 8.f);
            window.draw(closeCircle);
            sf::Text closeX("X", font, 16);
            closeX.setFillColor(textWhite);
            closeX.setPosition(winW - 36.f, 6.f);
            window.draw(closeX);
        } else {
            // Normal small screen layout
            // draw big owl? user requested big owl only in big screen, so do not draw big owl here

            // Title centered
            window.draw(titleText);

            // Timer
            window.draw(timerText);

            // Start button
            window.draw(startButton);
            window.draw(startText);

            // Progress bar gradient
            const int stripes = 80;
            for (int i = 0; i < stripes; ++i) {
                float x0 = barLeft + (barRight - barLeft) * (i / float(stripes));
                float x1 = barLeft + (barRight - barLeft) * ((i + 1) / float(stripes));
                float t = i / float(stripes - 1);
                sf::Uint8 r = (sf::Uint8)((1 - t) * 200 + t * 120);
                sf::Uint8 g = (sf::Uint8)((1 - t) * 0 + t * 0);
                sf::Uint8 b = (sf::Uint8)((1 - t) * 0 + t * 160);
                sf::RectangleShape stripe(sf::Vector2f(x1 - x0, barHeight));
                stripe.setPosition(x0, barY);
                stripe.setFillColor(sf::Color(r,g,b));
                window.draw(stripe);
            }
            // Outline
            sf::RectangleShape barOutline(sf::Vector2f(barRight - barLeft, barHeight));
            barOutline.setPosition(barLeft, barY);
            barOutline.setFillColor(sf::Color::Transparent);
            barOutline.setOutlineColor(sf::Color(80,0,0));
            barOutline.setOutlineThickness(2.f);
            window.draw(barOutline);

            // Checkpoints and moving owl
            for (int i = 0; i < totalCheckpoints; ++i) {
                float cx = checkpointX[i];
                checkpointOwl.setPosition(cx - 10.f, barY - 10.f);
                if (i < (totalCheckpoints - completedSessions)) window.draw(checkpointOwl);
                else {
                    sf::Sprite dim = checkpointOwl;
                    dim.setColor(sf::Color(80,80,80,160));
                    dim.setPosition(cx - 10.f, barY - 10.f);
                    window.draw(dim);
                }
            }
            window.draw(movingOwl);
        }

        // Draw settings panel overlay if visible (draw last so it's on top)
        if (showSettings) {
            window.draw(panel);
            // labels & values
            workLabel.setPosition(panel.getPosition().x + 20.f, panel.getPosition().y + 20.f);
            window.draw(workLabel);
            workValue.setPosition(panel.getPosition().x + 160.f, panel.getPosition().y + 18.f);
            window.draw(workValue);
            breakLabel.setPosition(panel.getPosition().x + 20.f, panel.getPosition().y + 56.f);
            window.draw(breakLabel);
            breakValue.setPosition(panel.getPosition().x + 160.f, panel.getPosition().y + 54.f);
            window.draw(breakValue);

            // plus/minus
            plusW.setPosition(panel.getPosition().x + 200.f, panel.getPosition().y + 18.f);
            minusW.setPosition(panel.getPosition().x + 240.f, panel.getPosition().y + 18.f);
            plusB.setPosition(panel.getPosition().x + 200.f, panel.getPosition().y + 56.f);
            minusB.setPosition(panel.getPosition().x + 240.f, panel.getPosition().y + 56.f);
            plusText.setPosition(plusW.getPosition().x + 6.f, plusW.getPosition().y + 2.f);
            minusText.setPosition(minusW.getPosition().x + 6.f, minusW.getPosition().y - 6.f);
            window.draw(plusW); window.draw(minusW); window.draw(plusText); window.draw(minusText);
            window.draw(plusB); window.draw(minusB); window.draw(plusText); // reuse plusText not ideal but fine

            // toggle
            toggleLabel.setPosition(panel.getPosition().x + 20.f, panel.getPosition().y + 96.f);
            toggleBox.setPosition(panel.getPosition().x + 200.f, panel.getPosition().y + 94.f);
            toggleOnOff.setPosition(toggleBox.getPosition().x + 6.f, toggleBox.getPosition().y + 1.f);
            window.draw(toggleLabel);
            window.draw(toggleBox);
            window.draw(toggleOnOff);

            // panel start
            panelStart.setPosition(panel.getPosition().x + 110.f, panel.getPosition().y + 118.f);
            panelStartText.setPosition(panelStart.getPosition().x + 8.f, panelStart.getPosition().y + 6.f);
            window.draw(panelStart);
            window.draw(panelStartText);
        }

        window.display();
    }

    return 0;
}

