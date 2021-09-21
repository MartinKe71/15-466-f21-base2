#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct BouncyCar: Mode {
	BouncyCar();
	virtual ~BouncyCar();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const& drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
		uint8_t released = 0;
	} space;

	struct GameCar {
		Scene::Transform* transform = nullptr;
		glm::vec3 velocity = glm::vec3(0.0f);
		glm::vec3 rotatingAxis;
		float angularSpeed;
		float jumpTime = 0.0f;
		float jumpStart = 0.0f;
		uint16_t bounceNum = 0;
		uint16_t totalDowns = 0;
		bool isGround = true;
	} gameCar;

	uint16_t score = 0;
	std::string scoreText = "Flying Distance: 0";
	std::string performanceText = "C";
	glm::u8vec4 textColor = glm::u8vec4(0xff, 0xff, 0xff, 0x00);

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform* car = nullptr;

	glm::quat car_base_rotation;
	glm::quat camera_base_rotation;

	//camera:
	Scene::Camera* camera = nullptr;

	//tiles
	std::vector<Scene::Transform*> tiles;

	//help functions
	void BounceCar();
	void CalculateScore();
};
