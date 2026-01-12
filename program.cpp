#include "splashkit.h"
#include <vector>
#include <string>

using namespace std;

enum ItemKind
{
    GOOD_FROG,
    GOOD_MUSHROOM,
    BAD_ROCK,
    BAD_SLIME,
    BONUS_POTION
};

struct Ingredient
{
    sprite item_sprite;
    ItemKind kind;
    double fall_speed;
};

enum GameState
{
    MENU,
    PLAYING,
    GAME_OVER
};

struct GameData
{
    sprite cauldron;
    vector<Ingredient> ingredients;
    int score;
    int high_score;
    int hp;
    int combo;
    int dash_cooldown;
    GameState state;
};

// PROTOTYPES

GameData new_game(int current_high = 0);
void handle_input(GameData &game);
void update_game(GameData &game);
void spawn_ingredient(GameData &game);
void check_collisions(GameData &game);
void cleanup_ingredients(GameData &game);
void draw_game(const GameData &game);

// MAIN GAME LOOP
int main()
{
    open_window("The Falling Alchemist", 800, 600);

    // Load Bitmaps
    load_bitmap("forest", "forest.png");
    load_bitmap("heart", "heart.png");
    load_bitmap("cauldron", "cauldron.png");
    load_bitmap("frog_eye", "frog_eye.png");
    load_bitmap("mushroom", "mushroom.png");
    load_bitmap("rock", "rock.png");
    load_bitmap("slime", "slime.png");
    load_bitmap("potion", "potion.png");

    // Load Music & Sounds
    load_music("bg_music", "bg_music.mp3");
    load_sound_effect("damage", "break.wav");
    load_sound_effect("success", "bubble.wav");
    load_sound_effect("dash", "dash.ogg");
    load_sound_effect("bonus", "bonus.ogg");
    load_sound_effect("lose", "lose.wav");

    play_music("bg_music", -1);
    set_music_volume(0.5);

    // Load custom font
    load_font("georgia", "georgia.ttf");

    GameData game = new_game(0);

    while (!quit_requested())
    {
        process_events();

        handle_input(game);
        update_game(game);

        clear_screen(COLOR_ALICE_BLUE);
        draw_game(game);
        refresh_screen(60);
    }
    return 0;
}

// GAME FUNCTIONS

GameData new_game(int current_high)
{
    GameData game;
    game.score = 0;
    game.high_score = current_high;
    game.hp = 3;
    game.combo = 0;
    game.dash_cooldown = 0;
    game.state = MENU;

    // Initialise Player
    game.cauldron = create_sprite("cauldron");
    sprite_set_x(game.cauldron, 400);
    sprite_set_y(game.cauldron, 500);

    return game;
}

void handle_input(GameData &game)
{
    if (game.state == MENU)
    {
        if (key_typed(SPACE_KEY))
        {
            game.state = PLAYING;
        }
    }
    else if (game.state == PLAYING)
    {
        int speed = 7;

        if (key_down(LEFT_SHIFT_KEY) && game.dash_cooldown <= 0) // Dash Mechanic
        {
            speed = 100;
            game.dash_cooldown = 40;
            play_sound_effect("dash");
        }
        if (key_down(LEFT_KEY) && sprite_x(game.cauldron) > 0) // Horizontal Movement
        {
            sprite_set_x(game.cauldron, sprite_x(game.cauldron) - speed);
        }
        if (key_down(RIGHT_KEY) && sprite_x(game.cauldron) < screen_width() - sprite_width(game.cauldron))
        {
            sprite_set_x(game.cauldron, sprite_x(game.cauldron) + speed);
        }
        if (sprite_x(game.cauldron) < 0) // Makes sure player is not out of bounds
        {
            sprite_set_x(game.cauldron, 0);
        }
        if (sprite_x(game.cauldron) > screen_width() - sprite_width(game.cauldron))
        {
            sprite_set_x(game.cauldron, screen_width() - sprite_width(game.cauldron));
        }
    }

    else if (game.state == GAME_OVER)
    {
        if (key_typed(R_KEY))
        {
            int current_top = game.high_score; // Update high score
            for (auto &ing : game.ingredients)
            {
                free_sprite(ing.item_sprite);
            }
            free_sprite(game.cauldron);
            game = new_game(current_top);
            play_music("bg_music", -1);
            set_music_volume(0.5);
        }
    }
}

void spawn_ingredient(GameData &game)
{
    // Random chance to spawn
    if (rnd() < 0.02)
    {
        Ingredient ing;
        int type_chance = rnd(1, 11);   // Determine category (Good, Bad, Bonus)
        int variety_chance = rnd(1, 3); // Determine specific visual variety

        // Good Category (60% Chance)
        if (type_chance <= 6)
        {
            if (variety_chance == 1)
            {
                ing.kind = GOOD_FROG;
                ing.item_sprite = create_sprite("frog_eye");
            }
            else
            {
                ing.kind = GOOD_MUSHROOM;
                ing.item_sprite = create_sprite("mushroom");
            }
        }
        // Bad Category (30% Chance)
        else if (type_chance <= 9)
        {
            if (variety_chance == 1)
            {
                ing.kind = BAD_ROCK;
                ing.item_sprite = create_sprite("rock");
            }
            else
            {
                ing.kind = BAD_SLIME;
                ing.item_sprite = create_sprite("slime");
            }
        }
        // Bonus Category (10% Chance)
        else
        {
            ing.kind = BONUS_POTION;
            ing.item_sprite = create_sprite("potion");
        }

        // Position sprite at top at a random X
        sprite_set_x(ing.item_sprite, rnd(0, screen_width() - sprite_width(ing.item_sprite)));
        sprite_set_y(ing.item_sprite, -50);
        ing.fall_speed = rnd(2, 6); // Random fall speed
        game.ingredients.push_back(ing);
    }
}
void update_game(GameData &game)
{
    if (game.state == PLAYING)
    {

        if (game.dash_cooldown > 0)
        {
            game.dash_cooldown--;
        }
        if (game.score > game.high_score)
        {
            game.high_score = game.score;
        }

        spawn_ingredient(game);

        // Update all ingredients (Gravity)
        for (int i = 0; i < game.ingredients.size(); i++)
        {
            sprite_set_y(game.ingredients[i].item_sprite, sprite_y(game.ingredients[i].item_sprite) + game.ingredients[i].fall_speed);
        }

        check_collisions(game);
        cleanup_ingredients(game);

        if (game.hp <= 0)
        {
            game.state = GAME_OVER;
            play_sound_effect("lose");
            stop_music();
        }
    }
}

void check_collisions(GameData &game)
{
    // Loop backwards when removing from vectors
    for (int i = game.ingredients.size() - 1; i >= 0; i--)
    {
        if (sprite_collision(game.cauldron, game.ingredients[i].item_sprite))
        {

            // Logic based on kind
            if (game.ingredients[i].kind == GOOD_FROG || game.ingredients[i].kind == GOOD_MUSHROOM)
            {
                game.combo++;
                game.score += (10 * game.combo);
                play_sound_effect("success");
            }
            else if (game.ingredients[i].kind == BAD_ROCK || game.ingredients[i].kind == BAD_SLIME)
            {
                game.hp -= 1;
                game.combo = 0;
                play_sound_effect("damage");
            }
            else if (game.ingredients[i].kind == BONUS_POTION)
            {
                game.score += 100;
                game.combo += 2;
                play_sound_effect("bonus");
            }

            // Remove sprite
            free_sprite(game.ingredients[i].item_sprite);
            game.ingredients.erase(game.ingredients.begin() + i);
        }
    }
}

void cleanup_ingredients(GameData &game)
{
    // Remove ingredients that fell off the bottom
    for (int i = game.ingredients.size() - 1; i >= 0; i--)
    {
        if (sprite_y(game.ingredients[i].item_sprite) > screen_height())
        {

            if (game.ingredients[i].kind == GOOD_FROG || game.ingredients[i].kind == GOOD_MUSHROOM)
            {
                game.combo = 0;
            }

            free_sprite(game.ingredients[i].item_sprite);
            game.ingredients.erase(game.ingredients.begin() + i);
        }
    }
}

void draw_game(const GameData &game)
{

    clear_screen(COLOR_BLACK);

    draw_bitmap("forest", 0, 0);

    if (game.state == MENU)
    {
        clear_screen(COLOR_STEEL_BLUE);

        // Title
        draw_text("THE FALLING", COLOR_GOLD, "georgia", 100, 60, 50);
        draw_text("ALCHEMIST", COLOR_GOLD, "georgia", 100, 80, 150);

        // Ingredient Legend
        int legend_y = 280;
        draw_text("THE ALCHEMIST'S GUIDE:", COLOR_WHITE, "georgia", 25, 100, legend_y);

        // Good Items
        draw_bitmap("frog_eye", 120, legend_y + 40);
        draw_bitmap("mushroom", 170, legend_y + 40);
        draw_text("= GOOD (Builds Combo)", COLOR_LIGHT_GREEN, "georgia", 18, 220, legend_y + 55);

        // Bad Items
        draw_bitmap("rock", 120, legend_y + 90);
        draw_bitmap("slime", 170, legend_y + 90);
        draw_text("= BAD (Breaks Combo & HP)", COLOR_LIGHT_PINK, "georgia", 18, 220, legend_y + 105);

        // Bonus
        draw_bitmap("potion", 120, legend_y + 140);
        draw_text("= BONUS (Massive Points)", COLOR_CYAN, "georgia", 18, 220, legend_y + 155);

        // Controls
        draw_text("ARROWS to Move | SHIFT to Dash", COLOR_WHITE, "georgia", 20, 220, 500);
        draw_text(">> PRESS SPACE TO BEGIN <<", COLOR_BLACK, "georgia", 30, 180, 540);
    }

    else
    {
        // Playing / Game Over Drawing
        draw_sprite(game.cauldron);
        for (const auto &ing : game.ingredients)
        {
            draw_sprite(ing.item_sprite);
        }

        // HUD
        fill_rectangle(rgba_color(0, 0, 0, 120), 5, 5, 230, 200);

        // Score & High Score
        draw_text("SCORE: " + to_string(game.score), COLOR_GOLD, "georgia", 18, 15, 15);
        draw_text("BEST:  " + to_string(game.high_score), COLOR_WHITE, "georgia", 16, 15, 40);

        // Lives (Hearts)
        draw_text("VITALITY:", COLOR_WHITE, "georgia", 16, 15, 70);

        for (int i = 0; i < game.hp; i++)
        {
            draw_bitmap("heart", 15 + (i * 45), 95);
        }

        // Combo Text
        if (game.combo > 1)
        {
            draw_text("COMBO x" + to_string(game.combo), COLOR_BRIGHT_GREEN, "georgia", 22, 15, 145);
        }

        // Dash Cooldown Bar
        double bar_width = 120;
        if (game.dash_cooldown > 0)
        {
            double progress = 1.0 - (game.dash_cooldown / 40.0);

            // Label
            draw_text("DASH:", COLOR_GRAY, "georgia", 14, 15, 175);

            // Bar Background
            fill_rectangle(COLOR_DARK_GRAY, 70, 178, bar_width, 10);
            // Bar Progress
            fill_rectangle(COLOR_ORANGE, 70, 178, bar_width * progress, 10);
        }
        else
        {
            draw_text("DASH READY!", COLOR_CYAN, "georgia", 14, 15, 175);
            // Full bar
            fill_rectangle(COLOR_CYAN, 120, 178, 70, 10);
        }

        // Game Over Overlay
        if (game.state == GAME_OVER)
        {
            draw_text("GAME OVER", COLOR_RED, "georgia", 80, 150, 200);
            draw_text("Final Score: " + to_string(game.score), COLOR_BLACK, "georgia", 30, 280, 310);
            draw_text("Press 'R' to Restart", COLOR_DARK_GRAY, "georgia", 20, 310, 360);
        }
    }
}