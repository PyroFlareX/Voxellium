#ifndef Menustate_H
#define Menustate_H

#include "../Application.h"

class Menustate : public Basestate
{
public:
    Menustate(Application& app);

	void createObject(bs::Transform& t, const std::string& name);
	void removeObject(const std::string& name);

    bool input(float dt) override;
    void update(float dt) override;
	void lateUpdate(Camera& cam) override;
	void render(Renderer& renderer) override;

    ~Menustate() override;
private:
	std::vector<bs::GameObject> m_gameObjects;
	
	Input::Inputs vInput;
};

#endif // Menustate_H
