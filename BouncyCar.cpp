#include "BouncyCar.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

#define GRAVITY 5.0f

GLuint car_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > car_meshes(LoadTagDefault, []() -> MeshBuffer const* {
	MeshBuffer const* ret = new MeshBuffer(data_path("car.pnct"));
	car_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
	});

Load< Scene > car_scene(LoadTagDefault, []() -> Scene const* {
	return new Scene(data_path("car.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
		Mesh const& mesh = car_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable& drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = car_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		});
	});

BouncyCar::BouncyCar() : scene(*car_scene) {
	//get pointers to leg for convenience:
	for (auto& transform : scene.transforms) {
		if (transform.name == "Car") gameCar.transform = &transform;
		else if (transform.name.find("Cube") != std::string::npos) {
			std::cout << transform.name << "\n";
			tiles.emplace_back(&transform);
		}
	}
	if (gameCar.transform == nullptr) throw std::runtime_error("car not found.");
	if (tiles.size() != 11) throw std::runtime_error("Failed to capture all tiles");
	std::sort(tiles.begin(), tiles.end(),
		[](Scene::Transform* a, Scene::Transform* b) {return a->position.y > b->position.y; });

	//for (auto& tile : tiles) {
	//	std::cout << "tile position " <<
	//		tile->position.x <<
	//		" " << tile->position.y <<
	//		" " << tile->position.z <<
	//		std::endl;
	//}

	std::cout << "Car scale " << gameCar.transform->scale.x << "\n";
	std::cout << "Car rotation: " <<
		gameCar.transform->rotation.w <<
		" " << gameCar.transform->position.x <<
		" " << gameCar.transform->position.y <<
		" " << gameCar.transform->position.z <<
		std::endl;

	gameCar.transform->position = glm::vec3(0.0f);
	car_base_rotation = gameCar.transform->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	//std::cout << "camera rotation: " <<
	//	camera->transform->rotation.w <<
	//	" " << camera->transform->position.x <<
	//	" " << camera->transform->position.y <<
	//	" " << camera->transform->position.z <<
	//	std::endl;
	std::cout << "camera fov: " << camera->fovy << "\n";
	camera_base_rotation = camera->transform->rotation;

	if (scene.lights.size() < 1) throw std::runtime_error("Failed to find light source!");
	light = &scene.lights.front();
}

BouncyCar::~BouncyCar() {
}

bool BouncyCar::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			space.released = false;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			space.released = true;
			return true;
		}
	}

	return false;
}

void BouncyCar::BounceCar() {
	if (gameCar.isGround) {
		gameCar.jumpStart = gameCar.transform->position.y;
	}

	gameCar.isGround = false;
	gameCar.bounceNum++;
	gameCar.totalDowns += space.downs;
	gameCar.velocity = glm::vec3(0.0f, 0.0f, GRAVITY) * gameCar.jumpTime
					+ glm::vec3(0.0f, -gameCar.totalDowns * 0.5f, 0.0f) ;

	int axisIdx = rand() % 3;
	float invert = (static_cast<float>(rand() % 2) - 0.5f) * 2.0f;
	gameCar.rotatingAxis = glm::vec3(1.0f, 0.0f, 0.0f) * invert;
	if (axisIdx == 1)
		gameCar.rotatingAxis = glm::vec3(0.0f, 1.0f, 0.0f) * invert;
	else if (axisIdx == 2)
		gameCar.rotatingAxis = glm::vec3(0.0f, 0.0f, 1.0f) * invert;

	uint8_t circle_num = (space.downs / 3);
	float angle = circle_num * glm::pi<float>() / gameCar.jumpTime;

	gameCar.angularSpeed = angle;
}

void BouncyCar::CalculateScore() {
	score = static_cast<uint16_t>(gameCar.jumpStart - gameCar.transform->position.y);

	scoreText = "Flying Distance: " + std::to_string(score);

	if (score > 2400) {
		textColor = glm::u8vec4(0xff, 0xd7, 0x00, 0xff);
		performanceText = "";
		for (uint8_t i = 0; i < score / 2400; i++) {
			performanceText += "S";
		}
	}
	else if (score > 900) {
		textColor = glm::u8vec4(0x2a, 0x22, 0xd6, 0xff);
		performanceText = "A";
	}
	else if (score > 300) {
		textColor = glm::u8vec4(0x42, 0x87, 0xe5, 0xff);
		performanceText = "B";
	}
}

void BouncyCar::update(float elapsed) {
	//move car
	{
		if (space.pressed) {
			if (space.downs > 15) space.downs = 15;
			gameCar.transform->scale = glm::vec3(1.0f - space.downs * 0.02f);
		}
		else if (space.released && space.downs > 3) {
			gameCar.jumpTime = space.downs * 0.1f;
			BounceCar();
			
			space.downs = 0;
			space.released = false;
		}

		if (!gameCar.isGround) {
			gameCar.transform->position += gameCar.velocity * elapsed;
			gameCar.velocity -= glm::vec3(0.0f, 0.0f, GRAVITY) * elapsed;
			gameCar.transform->rotation = glm::normalize(gameCar.transform->rotation 
									* glm::angleAxis(gameCar.angularSpeed * elapsed, gameCar.rotatingAxis));
			if (gameCar.transform->position.z > 7.0f) {
				gameCar.transform->position.z = 7.0f;
				gameCar.velocity.z *= -0.1f;
			}

			camera->transform->position += glm::vec3(0.0f, gameCar.velocity.y, 0.0f) * elapsed;


			if (gameCar.transform->scale.x < 1.0f) 
				gameCar.transform->scale += glm::vec3(elapsed);
			else 
				gameCar.transform->scale = glm::vec3(1.0f);

			
			CalculateScore();
			/*std::cout << "Car rotation: " <<
				gameCar.transform->rotation.w <<
				" " << gameCar.transform->position.x <<
				" " << gameCar.transform->position.y <<
				" " << gameCar.transform->position.z <<
				std::endl;*/
		}

		if (gameCar.transform->position.z < 0.0f) {
			gameCar.isGround = true;
			gameCar.transform->position.z = 0.0f;
			gameCar.transform->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			gameCar.totalDowns = 0;
			gameCar.velocity = glm::vec3(0.0f);
			gameCar.jumpTime = 0.0f;

			score = 0;
			scoreText = "Flying Distance: " + std::to_string(score);
			performanceText = "C";
			textColor = glm::u8vec4(0xff, 0xff, 0xff, 0x00);
		}
	}

	//move tiles
	{
		for (auto& tile : tiles) {
			if (tile->position.y > camera->transform->position.y) {
				/*Scene::transforms* tile_to_move = *tile;
				tile_to_move->position.y -= 72.0f;
				tiles.erase(tiles.begin());
				tiles.emplace_back(tile_to_move);*/
				tile->position.y -= 66.0f;

				std::sort(tiles.begin(), tiles.end(),
					[](Scene::Transform* a, Scene::Transform* b) {return a->position.y > b->position.y; });


				for (auto& tile : tiles) {
					std::cout << "tile position " <<
						tile->position.x <<
						" " << tile->position.y <<
						" " << tile->position.z <<
						std::endl;
				}
			}
		}		
	}
}

void BouncyCar::draw(glm::uvec2 const& drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, light->type);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::eulerAngles(light->transform->rotation) * -1.0f));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(light->energy));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Press SPACE bar to make the car FLYYYY",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Press SPACE bar to make the car FLYYYY",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw_text(scoreText,
			glm::vec3(aspect * 0.4f, -0.4f + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));		
		lines.draw_text(scoreText,
			glm::vec3(aspect  * 0.4f, -0.4f + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			textColor);

		lines.draw_text(performanceText,
			glm::vec3(aspect * 0.4f, -0.4f + 2.1f * H, 0.0),
			glm::vec3(2.0f * H, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f * H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(performanceText,
			glm::vec3(aspect * 0.4f + ofs, -0.4f + 2.1f * H + ofs, 0.0),
			glm::vec3(2.0f * H, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f * H, 0.0f),
			textColor);
	}
}