#include "Game.h"
#include "CommonIncludes.h"
#include "level/Level.h"
#include "Engine.h"

Game::Game() = default;
Game::~Game() = default;

void Game::init()
{
	level = std::make_unique<game::Level>(core::Engine::Get()->GetScene());
	level->load("resources/level/level1.mdl");
}

void Game::update(float dt)
{
	
}

void Game::cleanup()
{
	
}