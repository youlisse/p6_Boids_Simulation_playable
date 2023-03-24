#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
// #include "GLFW/glfw3.h"
#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "imgui.h"
#include "p6/p6.h"
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

struct paramRadius {
    float rAvoid    = 0.5f;
    float rCohesion = 0.5f;
    float rAlign    = 0.5f;
};
class boids {
private:
    glm::vec2 position;
    glm::vec2 acceleration; // acceleration
    glm::vec2 velocity;
    float     r{};
    float     r_cohesion{};
    float     r_align{};
    float     maxSpeed{};
    float     maxForce{};

public:
    float getX() const { return position.x; }
    float getY() const { return position.y; }
    float dirX() const { return velocity.x; }
    float dirY() const { return velocity.y; }
    float getR() const { return r; }
    float getRCohesion() const { return r_cohesion; }
    float getRAlign() const { return r_align; }
    void  setR(float value) { r = value; }
    void  setRCohesion(float value) { r_cohesion = value; }
    void  setRAlign(float value) { r_align = value; }

    explicit boids(p6::Context& context)
        : position(p6::random::point(context.aspect_ratio())), acceleration(glm::vec2(.0f)), velocity(p6::random::direction()), r(.045f), r_cohesion(.1f), r_align(.12f), maxSpeed(p6::random::number(0.005f, 0.02f)), maxForce(0.3)
    {
    }
    void refreshPos()
    {
        position += velocity * maxSpeed;
    }
    void controlBoids(p6::Context& context)
    {
        if (context.key_is_held(262)) // droite
        {
            velocity += glm::vec2(velocity.y / 7, -velocity.x / 7);
        }
        if (context.key_is_held(263)) // gauche
        {
            velocity += glm::vec2(-velocity.y / 7, velocity.x / 7);
        }
        checkOutOfBounce(context);
        velocity = glm::normalize(velocity);
        refreshPos();
    }

    void checkOutOfBounce(p6::Context& context)
    {
        if (position.x > context.aspect_ratio())
            position.x = -context.aspect_ratio();
        if (position.x < -context.aspect_ratio())
            position.x = context.aspect_ratio();
        if (position.y > 1)
            position.y = -1;
        if (position.y < -1)
            position.y = 1;
    }

    float distance_to(boids other_boid, p6::Context& context)
    {
        std::vector<float> tabDist;
        float              dx = other_boid.position[0] - position[0];
        float              dy = other_boid.position[1] - position[1];
        tabDist.push_back((std::sqrt(dx * dx + dy * dy)));
        tabDist.push_back((std::sqrt(dx * dx + (dy + 2) * (dy + 2))));
        tabDist.push_back((std::sqrt(dx * dx + (dy - 2) * (dy - 2))));
        dx += 2 * context.aspect_ratio();

        tabDist.push_back((std::sqrt(dx * dx + dy * dy)));
        tabDist.push_back((std::sqrt(dx * dx + (dy + 2) * (dy + 2))));
        tabDist.push_back((std::sqrt(dx * dx + (dy - 2) * (dy - 2))));
        dx -= 4 * context.aspect_ratio();

        tabDist.push_back((std::sqrt(dx * dx + dy * dy)));
        tabDist.push_back((std::sqrt(dx * dx + (dy + 2) * (dy + 2))));
        tabDist.push_back((std::sqrt(dx * dx + (dy - 2) * (dy - 2))));

        return *std::min_element(tabDist.begin(), tabDist.end());
    }

    glm::vec2 calculateSeparation(const std::vector<boids>& boidsList, p6::Context& context)
    {
        glm::vec2 separation = {0.0, 0.0};
        int       count      = 0;
        for (boids boid : boidsList)
        {
            if (&boid != this)
            {
                float distance = distance_to(boid, context);
                if (distance < r && distance > 0.0f)
                {
                    glm::vec2 diff = position - boid.position;
                    separation += glm::normalize(diff) / distance;
                    count += 1;
                }
            }
        }
        if (count > 0)
        {
            separation /= static_cast<float>(count);
        }
        return separation;
    }

    glm::vec2 calculateAlignment(const std::vector<boids>& boidsList, p6::Context& context)
    {
        glm::vec2 alignment(0.0f);
        int       count = 0;
        for (boids boid : boidsList)
        {
            if (&boid != this)
            {
                float distance = distance_to(boid, context);
                if (distance < r_align && distance > 0.0f)
                {
                    alignment += boid.velocity;
                    count += 1;
                }
            }
        }
        if (count > 0)
        {
            alignment /= static_cast<float>(count);
            alignment = glm::normalize(alignment);
        }
        return alignment;
    }

    glm::vec2 calculateCohesion(const std::vector<boids>& boidsList, p6::Context& context)
    {
        glm::vec2 cohesion(0.0f);
        int       count = 0;
        for (boids boid : boidsList)
        {
            if (&boid != this)
            {
                float distance = distance_to(boid, context);
                if (distance < r_cohesion && distance > r_cohesion * 0.50f && distance > 0.0f)
                {
                    cohesion += boid.position;
                    count += 1;
                }
            }
        }
        if (count > 0)
        {
            cohesion /= static_cast<float>(count);
            cohesion -= position;
            cohesion = glm::normalize(cohesion);
        }
        return cohesion;
    }

    void update(const std::vector<boids>& boidsList, p6::Context& context)
    {
        // Calculate the separation, alignment, and cohesion vectors
        glm::vec2 separation = calculateSeparation(boidsList, context);
        glm::vec2 alignment  = calculateAlignment(boidsList, context);
        glm::vec2 cohesion   = calculateCohesion(boidsList, context);

        // Add the three vectors together to get the desired direction
        glm::vec2 desiredDirection = separation + alignment + cohesion;

        // Limit the desired direction to the maximum speed
        if (desiredDirection != glm::vec2(0.f))
            desiredDirection = glm::normalize(desiredDirection);

        // Calculate the steering force required to achieve the desired direction
        auto steering = glm::vec2(0.f);
        if (glm::abs((desiredDirection).x - velocity.x) > maxForce || glm::abs((desiredDirection).y - velocity.y) > maxForce)
        {
            steering = glm::vec2(desiredDirection.x * maxForce, desiredDirection.y * maxForce);
        }
        else
        {
            steering = (desiredDirection);
        }

        // Apply the steering force to the acceleration
        acceleration += steering;

        // Update the velocity and position based on the acceleration
        velocity += acceleration;
        // Limit the speed to the maximum speed
        /*if (velocity.x > maxSpeed)
            velocity = glm::vec2(maxSpeed, velocity.y);

        if (velocity.y > maxSpeed)
            velocity = glm::vec2(velocity.x, maxSpeed);*/
        // std::cout << velocity.x << velocity.y << std::endl;
        velocity = glm::normalize(velocity);

        // Reset the acceleration to zero for the next update
        acceleration *= 0;
    }
};

class Flock {
private:
    std::vector<boids> boidsList;

public:
    std::vector<boids> getList() { return boidsList; }
    Flock()
        : boidsList(*new std::vector<boids>())
    {
    }

    void refreshBoids(p6::Context& context)
    {
        for (boids& b : boidsList)
        {
            b.refreshPos();
            b.checkOutOfBounce(context);
        }
    }
    void flocking(p6::Context& context)
    {
        for (boids& b : boidsList)
        {
            b.update(boidsList, context);
        }
    }
    void flocking(p6::Context& context, boids b)
    {
        std::vector<boids> boidsListPlusMe = boidsList;
        boidsListPlusMe.push_back(b);
        for (boids& b : boidsList)
        {
            b.update(boidsListPlusMe, context);
        }
    }

    void addBoids(boids boids)
    {
        boidsList.push_back(boids);
    }

    void initBoids(int nbElem, p6::Context& context)
    {
        for (int i = 0; i < nbElem; i++)
        {
            boids b(context);
            addBoids(b);
        }
    }
    void refreshParam(paramRadius para)
    {
        for (boids& b : boidsList)
        {
            b.setR(para.rAvoid);
            b.setRAlign(para.rAlign);
            b.setRCohesion(para.rCohesion);
        }
    }
};

void drawBoids(const std::vector<boids>& listedPosition, p6::Context& context)
{
    float length       = 0.03;
    float thickness    = 0.02;
    context.use_fill   = true;
    context.use_stroke = false;
    context.fill       = {0.05f, 0.9f, 0.4f, 1.0f};
    for (auto b : listedPosition)
        context.triangle(p6::Point2D((b.dirX()) * length + b.getX(), (b.dirY()) * length + b.getY()), p6::Point2D((b.getX() + (b.dirY()) * thickness) - ((b.dirX()) * length), (b.getY() - (b.dirX()) * thickness) - ((b.dirY()) * length)), p6::Point2D((b.getX() - (b.dirY()) * thickness) - ((b.dirX()) * length), (b.getY() + (b.dirX()) * thickness) - ((b.dirY()) * length)));
}
void drawBoids(boids boid, p6::Context& context)
{
    float length       = 0.03;
    float thickness    = 0.025;
    context.use_fill   = true;
    context.use_stroke = false;
    context.fill       = {0.9f, 0.2f, 0.1f, 1.0f};
    context.triangle(p6::Point2D((boid.dirX()) * length + boid.getX(), (boid.dirY()) * length + boid.getY()), p6::Point2D((boid.getX() + (boid.dirY()) * thickness) - ((boid.dirX()) * length), (boid.getY() - (boid.dirX()) * thickness) - ((boid.dirY()) * length)), p6::Point2D((boid.getX() - (boid.dirY()) * thickness) - ((boid.dirX()) * length), (boid.getY() + (boid.dirX()) * thickness) - ((boid.dirY()) * length)));
}

void drawRadius(const std::vector<boids>& listedPosition, p6::Context& context)
{
    context.use_stroke = false;
    for (auto b : listedPosition)
    {
        context.use_stroke = true;

        context.use_fill = false;
        context.stroke   = {0.1f, 0.3f, 0.4f, 1.0f};
        context.circle(glm::vec2(b.getX(), b.getY()), b.getR());
        context.stroke = {0.6f, 0.2f, 0.9f, 1.0f};
        context.circle(glm::vec2(b.getX(), b.getY()), b.getRAlign());
        context.stroke = {0.9f, 0.7f, 0.1f, 1.0f};
        context.circle(glm::vec2(b.getX(), b.getY()), b.getRCohesion());
    }
}
void drawRadius(const boids boid, p6::Context& context)
{
    context.use_stroke = false;
    context.use_stroke = true;
    context.use_fill   = false;
    context.stroke     = {0.1f, 0.3f, 0.4f, 1.0f};
    context.circle(glm::vec2(boid.getX(), boid.getY()), boid.getR());
    context.stroke = {0.6f, 0.2f, 0.9f, 1.0f};
    context.circle(glm::vec2(boid.getX(), boid.getY()), boid.getRAlign());
    context.stroke = {0.9f, 0.7f, 0.1f, 1.0f};
    context.circle(glm::vec2(boid.getX(), boid.getY()), boid.getRCohesion());
}

////////////

int main(int argc, char* argv[])
{
    { // Run the tests
        if (doctest::Context{}.run() != 0)
            return EXIT_FAILURE;
        // The CI does not have a GPU so it cannot run the rest of the code.
        const bool no_gpu_available = argc >= 2 && strcmp(argv[1], "-nogpu") == 0; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (no_gpu_available)
            return EXIT_SUCCESS;
    }

    //  Actual app
    auto ctx = p6::Context{{.title = "Simple-p6-Setup"}};
    // ctx.framerate_synced_with_monitor();
    Flock f = *new Flock();
    f.initBoids(30, ctx);
    auto myBoid = boids(ctx);
    ctx.maximize_window();

    paramRadius para;
    float       alpha = 0.5f;

    bool radius_show = false;
    ctx.imgui        = [&]() {
        ImGui::Begin("param");
        ImGui::SliderFloat("Alpha", &alpha, 0.f, 1.f);
        ImGui::SliderFloat("avoidRadius", &para.rAvoid, 0.f, 1.f);
        ImGui::SliderFloat("cohesionRadius", &para.rCohesion, 0.f, 1.f);
        ImGui::SliderFloat("alignRadius", &para.rAlign, 0.f, 1.f);

        if (ImGui::Button("Show Radius"))
            radius_show = !radius_show;
        ImGui::End();
    };

    //  Declare your infinite update loop.
    ctx.update = [&]() {
        myBoid.controlBoids(ctx);
        f.flocking(ctx, myBoid);
        f.refreshBoids(ctx);
        f.refreshParam(para);
        myBoid.setR(para.rAvoid);
        myBoid.setRAlign(para.rAlign);
        myBoid.setRCohesion(para.rCohesion);
        ctx.fill = {0.f, 0.f, 0.f, alpha};
        ctx.rectangle(p6::Center(), glm::vec2(ctx.aspect_ratio()), p6::Angle());

        drawBoids(f.getList(), ctx);
        if (radius_show)
            drawRadius(myBoid, ctx);
        drawBoids(myBoid, ctx);
    };

    // Should be done last. It starts the infinite loop.
    ctx.start();
}