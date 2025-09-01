// File: tests/unit_tests/template_tests/ConceptTests.cpp
// C++20 concept tests for CppVerseHub template programming showcase

#include <catch2/catch.hpp>
#include <concepts>
#include <type_traits>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// Include the concept showcase headers
#include "ConceptsDemo.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Entity.hpp"

using namespace CppVerseHub::Modern;
using namespace CppVerseHub::Core;

// Custom concepts for testing
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept HasName = requires(T t) {
    { t.getName() } -> std::convertible_to<std::string>;
};

template<typename T>
concept HasId = requires(T t) {
    { t.getId() } -> std::convertible_to<int>;
};

template<typename T>
concept Entity_Concept = HasName<T> && HasId<T> && requires(T t) {
    { t.update(std::declval<double>()) } -> std::same_as<void>;
    { t.getHealth() } -> std::convertible_to<double>;
};

template<typename T>
concept Container_Concept = requires(T t) {
    typename T::value_type;
    typename T::iterator;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
};

template<typename T>
concept RandomAccessIterator_Concept = requires(T it) {
    { it + 1 } -> std::same_as<T>;
    { it - 1 } -> std::same_as<T>;
    { it[0] };
    { it < it } -> std::convertible_to<bool>;
};

TEST_CASE("Basic C++20 Concepts", "[concepts][c++20][basic]") {
    
    SECTION("Standard Library Concepts") {
        SECTION("Arithmetic concepts") {
            static_assert(std::integral<int>);
            static_assert(std::integral<long>);
            static_assert(std::integral<char>);
            static_assert(!std::integral<double>);
            static_assert(!std::integral<std::string>);
            
            static_assert(std::floating_point<float>);
            static_assert(std::floating_point<double>);
            static_assert(!std::floating_point<int>);
            
            static_assert(std::signed_integral<int>);
            static_assert(!std::signed_integral<unsigned int>);
            static_assert(std::unsigned_integral<unsigned int>);
            
            REQUIRE(std::integral<int>);
            REQUIRE(std::floating_point<double>);
            REQUIRE(!std::integral<double>);
        }
        
        SECTION("Iterator concepts") {
            static_assert(std::input_iterator<std::vector<int>::iterator>);
            static_assert(std::forward_iterator<std::vector<int>::iterator>);
            static_assert(std::bidirectional_iterator<std::vector<int>::iterator>);
            static_assert(std::random_access_iterator<std::vector<int>::iterator>);
            
            static_assert(std::input_iterator<std::list<int>::iterator>);
            static_assert(std::forward_iterator<std::list<int>::iterator>);
            static_assert(std::bidirectional_iterator<std::list<int>::iterator>);
            static_assert(!std::random_access_iterator<std::list<int>::iterator>);
            
            REQUIRE(std::random_access_iterator<std::vector<int>::iterator>);
            REQUIRE(!std::random_access_iterator<std::list<int>::iterator>);
        }
        
        SECTION("Range concepts") {
            static_assert(std::ranges::range<std::vector<int>>);
            static_assert(std::ranges::range<std::string>);
            static_assert(std::ranges::range<std::array<int, 5>>);
            static_assert(!std::ranges::range<int>);
            
            static_assert(std::ranges::random_access_range<std::vector<int>>);
            static_assert(std::ranges::random_access_range<std::string>);
            static_assert(!std::ranges::random_access_range<std::list<int>>);
            
            REQUIRE(std::ranges::range<std::vector<int>>);
            REQUIRE(!std::ranges::range<int>);
        }
        
        SECTION("Callable concepts") {
            auto lambda = [](int x) { return x * 2; };
            auto function_ptr = [](int x) -> int { return x * 3; };
            
            static_assert(std::invocable<decltype(lambda), int>);
            static_assert(std::invocable<decltype(function_ptr), int>);
            static_assert(!std::invocable<decltype(lambda), std::string>);
            
            static_assert(std::regular_invocable<decltype(lambda), int>);
            static_assert(std::predicate<decltype([](int x) { return x > 0; }), int>);
            
            REQUIRE(std::invocable<decltype(lambda), int>);
            REQUIRE(!std::invocable<decltype(lambda), std::string>);
        }
    }
    
    SECTION("Custom Concepts") {
        SECTION("Numeric concept") {
            static_assert(Numeric<int>);
            static_assert(Numeric<double>);
            static_assert(Numeric<float>);
            static_assert(Numeric<long>);
            static_assert(!Numeric<std::string>);
            static_assert(!Numeric<Planet>);
            
            REQUIRE(Numeric<int>);
            REQUIRE(Numeric<double>);
            REQUIRE(!Numeric<std::string>);
        }
        
        SECTION("HasName concept") {
            static_assert(HasName<Planet>);
            static_assert(HasName<Fleet>);
            static_assert(!HasName<int>);
            static_assert(!HasName<std::string>);
            
            REQUIRE(HasName<Planet>);
            REQUIRE(HasName<Fleet>);
            REQUIRE(!HasName<int>);
        }
        
        SECTION("Entity concept") {
            static_assert(Entity_Concept<Planet>);
            static_assert(Entity_Concept<Fleet>);
            static_assert(!Entity_Concept<int>);
            static_assert(!Entity_Concept<std::string>);
            
            REQUIRE(Entity_Concept<Planet>);
            REQUIRE(Entity_Concept<Fleet>);
            REQUIRE(!Entity_Concept<int>);
        }
        
        SECTION("Container concept") {
            static_assert(Container_Concept<std::vector<int>>);
            static_assert(Container_Concept<std::list<int>>);
            static_assert(Container_Concept<std::string>);
            static_assert(!Container_Concept<int>);
            static_assert(!Container_Concept<Planet>);
            
            REQUIRE(Container_Concept<std::vector<int>>);
            REQUIRE(!Container_Concept<int>);
        }
    }
}

TEST_CASE("Concept-Constrained Functions", "[concepts][functions][constraints]") {
    
    SECTION("Functions with concept constraints") {
        // Function that only accepts numeric types
        auto add_numeric = []<Numeric T>(T a, T b) {
            return a + b;
        };
        
        // Function that only accepts entities
        auto get_entity_info = []<Entity_Concept E>(const E& entity) {
            return entity.getName() + " (ID: " + std::to_string(entity.getId()) + ")";
        };
        
        // Function that only accepts containers
        auto get_container_size = []<Container_Concept C>(const C& container) {
            return container.size();
        };
        
        SECTION("Numeric function tests") {
            REQUIRE(add_numeric(5, 3) == 8);
            REQUIRE(add_numeric(2.5, 1.5) == Approx(4.0));
            REQUIRE(add_numeric(10L, 20L) == 30L);
            
            // These would not compile:
            // add_numeric("hello", "world");  // Error: string is not Numeric
            // add_numeric(Planet{...}, Fleet{...});  // Error: Entity is not Numeric
        }
        
        SECTION("Entity function tests") {
            Planet testPlanet("TestPlanet", Vector3D{0, 0, 0});
            Fleet testFleet("TestFleet", Vector3D{0, 0, 0});
            
            std::string planetInfo = get_entity_info(testPlanet);
            std::string fleetInfo = get_entity_info(testFleet);
            
            REQUIRE(planetInfo.find("TestPlanet") != std::string::npos);
            REQUIRE(planetInfo.find("ID:") != std::string::npos);
            REQUIRE(fleetInfo.find("TestFleet") != std::string::npos);
            
            // These would not compile:
            // get_entity_info(42);  // Error: int is not Entity_Concept
            // get_entity_info("string");  // Error: string is not Entity_Concept
        }
        
        SECTION("Container function tests") {
            std::vector<int> vec = {1, 2, 3, 4, 5};
            std::string str = "hello";
            std::list<double> lst = {1.1, 2.2, 3.3};
            
            REQUIRE(get_container_size(vec) == 5);
            REQUIRE(get_container_size(str) == 5);
            REQUIRE(get_container_size(lst) == 3);
            
            // These would not compile:
            // get_container_size(42);  // Error: int is not Container_Concept
        }
    }
    
    SECTION("Overload resolution with concepts") {
        // Multiple overloads with different concept constraints
        auto process_value = []<std::integral T>(T value) {
            return "integer: " + std::to_string(value);
        };
        
        auto process_value_float = []<std::floating_point T>(T value) {
            return "floating: " + std::to_string(value);
        };
        
        auto process_value_entity = []<Entity_Concept T>(const T& entity) {
            return "entity: " + entity.getName();
        };
        
        SECTION("Concept-based overload selection") {
            REQUIRE(process_value(42) == "integer: 42");
            REQUIRE(process_value_float(3.14) == "floating: 3.140000");
            
            Planet testPlanet("OverloadPlanet", Vector3D{0, 0, 0});
            REQUIRE(process_value_entity(testPlanet) == "entity: OverloadPlanet");
        }
    }
    
    SECTION("Concept subsumption") {
        // More specific concept subsumes less specific ones
        template<typename T>
        concept SignedIntegral = std::integral<T> && std::signed_integral<T>;
        
        template<typename T>
        concept UnsignedIntegral = std::integral<T> && std::unsigned_integral<T>;
        
        auto handle_integral = []<std::integral T>(T value) {
            return "general integral: " + std::to_string(value);
        };
        
        auto handle_signed = []<SignedIntegral T>(T value) {
            return "signed integral: " + std::to_string(value);
        };
        
        auto handle_unsigned = []<UnsignedIntegral T>(T value) {
            return "unsigned integral: " + std::to_string(value);
        };
        
        // Test subsumption - more specific concepts are preferred
        REQUIRE(handle_signed(-42) == "signed integral: -42");
        REQUIRE(handle_unsigned(42u) == "unsigned integral: 42");
        
        // If we had only the general version, it would be used
        // But with subsumption, the more specific version is chosen
    }
}

TEST_CASE("Concept-Based Class Templates", "[concepts][templates][classes]") {
    
    SECTION("Class template with concept constraints") {
        template<Entity_Concept T>
        class EntityManager {
        private:
            std::vector<T> entities_;
            
        public:
            void add(const T& entity) {
                entities_.push_back(entity);
            }
            
            void add(T&& entity) {
                entities_.push_back(std::move(entity));
            }
            
            std::size_t count() const {
                return entities_.size();
            }
            
            std::vector<std::string> getNames() const {
                std::vector<std::string> names;
                for (const auto& entity : entities_) {
                    names.push_back(entity.getName());
                }
                return names;
            }
            
            std::vector<int> getIds() const {
                std::vector<int> ids;
                for (const auto& entity : entities_) {
                    ids.push_back(entity.getId());
                }
                return ids;
            }
            
            void updateAll(double deltaTime) {
                for (auto& entity : entities_) {
                    entity.update(deltaTime);
                }
            }
            
            double getTotalHealth() const {
                double total = 0.0;
                for (const auto& entity : entities_) {
                    total += entity.getHealth();
                }
                return total;
            }
        };
        
        SECTION("EntityManager with Planets") {
            EntityManager<Planet> planetManager;
            
            Planet planet1("Planet1", Vector3D{100, 100, 100});
            Planet planet2("Planet2", Vector3D{200, 200, 200});
            
            planetManager.add(planet1);
            planetManager.add(std::move(planet2));
            
            REQUIRE(planetManager.count() == 2);
            
            auto names = planetManager.getNames();
            REQUIRE(names.size() == 2);
            REQUIRE(std::find(names.begin(), names.end(), "Planet1") != names.end());
            REQUIRE(std::find(names.begin(), names.end(), "Planet2") != names.end());
            
            auto ids = planetManager.getIds();
            REQUIRE(ids.size() == 2);
            REQUIRE(ids[0] > 0);
            REQUIRE(ids[1] > 0);
            REQUIRE(ids[0] != ids[1]);
            
            double totalHealth = planetManager.getTotalHealth();
            REQUIRE(totalHealth == Approx(200.0)); // 100 + 100
            
            planetManager.updateAll(1.0);
            // Entities should still be healthy after update
            REQUIRE(planetManager.getTotalHealth() == Approx(200.0));
        }
        
        SECTION("EntityManager with Fleets") {
            EntityManager<Fleet> fleetManager;
            
            Fleet fleet1("Fleet1", Vector3D{300, 300, 300});
            Fleet fleet2("Fleet2", Vector3D{400, 400, 400});
            
            fleet1.addShips(ShipType::FIGHTER, 10);
            fleet2.addShips(ShipType::CRUISER, 5);
            
            fleetManager.add(fleet1);
            fleetManager.add(std::move(fleet2));
            
            REQUIRE(fleetManager.count() == 2);
            
            auto names = fleetManager.getNames();
            REQUIRE(names.size() == 2);
            REQUIRE(std::find(names.begin(), names.end(), "Fleet1") != names.end());
            REQUIRE(std::find(names.begin(), names.end(), "Fleet2") != names.end());
            
            // Test that fleets can be updated
            fleetManager.updateAll(2.0);
            REQUIRE(fleetManager.getTotalHealth() == Approx(200.0));
        }
        
        // This would not compile - int does not satisfy Entity_Concept:
        // EntityManager<int> intManager;  // Compilation error
    }
    
    SECTION("Generic container with concept constraints") {
        template<Container_Concept C>
        class ContainerAnalyzer {
        private:
            const C& container_;
            
        public:
            explicit ContainerAnalyzer(const C& container) : container_(container) {}
            
            std::size_t size() const {
                return container_.size();
            }
            
            bool empty() const {
                return container_.empty();
            }
            
            std::string getTypeInfo() const {
                if constexpr (std::ranges::random_access_range<C>) {
                    return "random_access_container";
                } else if constexpr (std::ranges::bidirectional_range<C>) {
                    return "bidirectional_container";
                } else if constexpr (std::ranges::forward_range<C>) {
                    return "forward_container";
                } else {
                    return "input_container";
                }
            }
            
            // Only available for containers with numeric value types
            template<typename T = typename C::value_type>
            requires Numeric<T>
            T sum() const {
                T total{};
                for (const auto& element : container_) {
                    total += element;
                }
                return total;
            }
        };
        
        SECTION("ContainerAnalyzer with different containers") {
            std::vector<int> vec = {1, 2, 3, 4, 5};
            std::list<int> lst = {10, 20, 30};
            std::string str = "hello";
            
            ContainerAnalyzer vecAnalyzer(vec);
            ContainerAnalyzer lstAnalyzer(lst);
            ContainerAnalyzer strAnalyzer(str);
            
            REQUIRE(vecAnalyzer.size() == 5);
            REQUIRE(lstAnalyzer.size() == 3);
            REQUIRE(strAnalyzer.size() == 5);
            
            REQUIRE(vecAnalyzer.getTypeInfo() == "random_access_container");
            REQUIRE(lstAnalyzer.getTypeInfo() == "bidirectional_container");
            REQUIRE(strAnalyzer.getTypeInfo() == "random_access_container");
            
            // Test sum for numeric containers
            REQUIRE(vecAnalyzer.sum() == 15);
            REQUIRE(lstAnalyzer.sum() == 60);
            
            // String sum would sum character values
            auto strSum = strAnalyzer.sum();
            REQUIRE(strSum > 0); // Sum of ASCII values
        }
    }
}

TEST_CASE("Advanced Concept Patterns", "[concepts][advanced][patterns]") {
    
    SECTION("Concept composition and refinement") {
        // Base concepts
        template<typename T>
        concept Drawable = requires(T t) {
            t.draw();
        };
        
        template<typename T>
        concept Updateable = requires(T t) {
            t.update(std::declval<double>());
        };
        
        template<typename T>
        concept Movable = requires(T t) {
            { t.getPosition() } -> std::convertible_to<Vector3D>;
            t.setPosition(std::declval<Vector3D>());
        };
        
        // Composed concepts
        template<typename T>
        concept GameObject = Drawable<T> && Updateable<T> && Movable<T>;
        
        template<typename T>
        concept SimulationObject = GameObject<T> && HasName<T> && HasId<T>;
        
        // Test concept satisfaction
        static_assert(Drawable<Planet>);
        static_assert(Updateable<Planet>);
        static_assert(Movable<Planet>);
        static_assert(GameObject<Planet>);
        static_assert(SimulationObject<Planet>);
        
        static_assert(Drawable<Fleet>);
        static_assert(Updateable<Fleet>);
        static_assert(Movable<Fleet>);
        static_assert(GameObject<Fleet>);
        static_assert(SimulationObject<Fleet>);
        
        REQUIRE(GameObject<Planet>);
        REQUIRE(SimulationObject<Fleet>);
    }
    
    SECTION("Concept-based algorithms") {
        // Algorithm that works with any drawable objects
        template<std::ranges::range R>
        requires Drawable<std::ranges::range_value_t<R>>
        void draw_all(R&& range) {
            for (auto& item : range) {
                item.draw();
            }
        }
        
        // Algorithm that works with any updateable objects
        template<std::ranges::range R>
        requires Updateable<std::ranges::range_value_t<R>>
        void update_all(R&& range, double deltaTime) {
            for (auto& item : range) {
                item.update(deltaTime);
            }
        }
        
        // Algorithm that works with any movable objects
        template<std::ranges::range R>
        requires Movable<std::ranges::range_value_t<R>>
        Vector3D get_center_of_mass(R&& range) {
            if (std::ranges::empty(range)) {
                return Vector3D{0, 0, 0};
            }
            
            Vector3D sum{0, 0, 0};
            for (const auto& item : range) {
                Vector3D pos = item.getPosition();
                sum.x += pos.x;
                sum.y += pos.y;
                sum.z += pos.z;
            }
            
            auto count = static_cast<double>(std::ranges::size(range));
            return Vector3D{sum.x / count, sum.y / count, sum.z / count};
        }
        
        SECTION("Test concept-based algorithms") {
            std::vector<Planet> planets;
            planets.emplace_back("Planet1", Vector3D{0, 0, 0});
            planets.emplace_back("Planet2", Vector3D{100, 100, 100});
            planets.emplace_back("Planet3", Vector3D{200, 200, 200});
            
            std::vector<Fleet> fleets;
            fleets.emplace_back("Fleet1", Vector3D{50, 50, 50});
            fleets.emplace_back("Fleet2", Vector3D{150, 150, 150});
            
            // Test draw_all
            REQUIRE_NOTHROW(draw_all(planets));
            REQUIRE_NOTHROW(draw_all(fleets));
            
            // Test update_all
            REQUIRE_NOTHROW(update_all(planets, 1.0));
            REQUIRE_NOTHROW(update_all(fleets, 1.0));
            
            // Test get_center_of_mass
            Vector3D planetCenter = get_center_of_mass(planets);
            REQUIRE(planetCenter.x == Approx(100.0));
            REQUIRE(planetCenter.y == Approx(100.0));
            REQUIRE(planetCenter.z == Approx(100.0));
            
            Vector3D fleetCenter = get_center_of_mass(fleets);
            REQUIRE(fleetCenter.x == Approx(100.0));
            REQUIRE(fleetCenter.y == Approx(100.0));
            REQUIRE(fleetCenter.z == Approx(100.0));
        }
    }
    
    SECTION("Concept-based factory pattern") {
        template<Entity_Concept T>
        class ConceptFactory {
        public:
            template<typename... Args>
            requires std::constructible_from<T, Args...>
            static std::unique_ptr<T> create(Args&&... args) {
                return std::make_unique<T>(std::forward<Args>(args)...);
            }
            
            template<typename... Args>
            requires std::constructible_from<T, Args...>
            static std::vector<std::unique_ptr<T>> createBatch(std::size_t count, Args&&... args) {
                std::vector<std::unique_ptr<T>> batch;
                batch.reserve(count);
                
                for (std::size_t i = 0; i < count; ++i) {
                    batch.push_back(std::make_unique<T>(args...));
                }
                
                return batch;
            }
            
            static std::vector<std::string> getEntityNames(const std::vector<std::unique_ptr<T>>& entities) {
                std::vector<std::string> names;
                names.reserve(entities.size());
                
                for (const auto& entity : entities) {
                    names.push_back(entity->getName());
                }
                
                return names;
            }
        };
        
        SECTION("Planet factory") {
            using PlanetFactory = ConceptFactory<Planet>;
            
            auto planet = PlanetFactory::create("FactoryPlanet", Vector3D{300, 300, 300});
            REQUIRE(planet != nullptr);
            REQUIRE(planet->getName() == "FactoryPlanet");
            
            auto planetBatch = PlanetFactory::createBatch(3, "BatchPlanet", Vector3D{400, 400, 400});
            REQUIRE(planetBatch.size() == 3);
            
            for (const auto& p : planetBatch) {
                REQUIRE(p->getName() == "BatchPlanet");
            }
            
            auto names = PlanetFactory::getEntityNames(planetBatch);
            REQUIRE(names.size() == 3);
            REQUIRE(std::all_of(names.begin(), names.end(), 
                              [](const std::string& name) { return name == "BatchPlanet"; }));
        }
        
        SECTION("Fleet factory") {
            using FleetFactory = ConceptFactory<Fleet>;
            
            auto fleet = FleetFactory::create("FactoryFleet", Vector3D{500, 500, 500});
            REQUIRE(fleet != nullptr);
            REQUIRE(fleet->getName() == "FactoryFleet");
            
            auto fleetBatch = FleetFactory::createBatch(2, "BatchFleet", Vector3D{600, 600, 600});
            REQUIRE(fleetBatch.size() == 2);
            
            auto names = FleetFactory::getEntityNames(fleetBatch);
            REQUIRE(names.size() == 2);
            REQUIRE(std::all_of(names.begin(), names.end(), 
                              [](const std::string& name) { return name == "BatchFleet"; }));
        }
    }
}

TEST_CASE("Concept Performance and Compilation", "[concepts][performance][compilation]") {
    
    SECTION("Concept evaluation performance") {
        // Concepts are evaluated at compile time, so runtime performance is not affected
        template<Entity_Concept T>
        void process_entities(const std::vector<T>& entities) {
            for (const auto& entity : entities) {
                // Concept constraint ensures this is safe
                entity.getName();
                entity.getId();
                // No runtime type checking needed
            }
        }
        
        std::vector<Planet> planets;
        for (int i = 0; i < 1000; ++i) {
            planets.emplace_back("Planet" + std::to_string(i), Vector3D{i, i, i});
        }
        
        auto duration = measurePerformance([&]() {
            process_entities(planets);
        }, "Processing entities with concept constraints");
        
        // Should be fast since no runtime type checking
        REQUIRE(duration.count() < 100000); // 100ms threshold
        
        INFO("Concept-constrained processing took: " << duration.count() << " microseconds");
    }
    
    SECTION("Concept vs SFINAE compilation comparison") {
        // This demonstrates the clarity advantage of concepts over SFINAE
        // Both achieve the same result, but concepts are more readable
        
        // SFINAE version (old style)
        template<typename T>
        auto sfinae_process(T&& t) -> 
            std::enable_if_t<
                std::is_same_v<decltype(t.getName()), std::string> &&
                std::is_same_v<decltype(t.getId()), int>,
                std::string
            > {
            return t.getName() + " (ID: " + std::to_string(t.getId()) + ")";
        }
        
        // Concept version (modern C++20)
        template<Entity_Concept T>
        std::string concept_process(T&& t) {
            return t.getName() + " (ID: " + std::to_string(t.getId()) + ")";
        }
        
        Planet testPlanet("TestPlanet", Vector3D{0, 0, 0});
        Fleet testFleet("TestFleet", Vector3D{0, 0, 0});
        
        // Both should work the same way
        REQUIRE(sfinae_process(testPlanet) == concept_process(testPlanet));
        REQUIRE(sfinae_process(testFleet) == concept_process(testFleet));
        
        // But concepts provide better error messages and are more readable
        REQUIRE(concept_process(testPlanet).find("TestPlanet") != std::string::npos);
        REQUIRE(concept_process(testFleet).find("TestFleet") != std::string::npos);
    }
    
    SECTION("Complex concept evaluation") {
        // Test that complex concepts don't significantly impact compilation
        template<typename T>
        concept ComplexConcept = 
            std::is_default_constructible_v<T> &&
            std::is_copy_constructible_v<T> &&
            std::is_move_constructible_v<T> &&
            std::is_destructible_v<T> &&
            HasName<T> &&
            HasId<T> &&
            requires(T t) {
                { t.update(1.0) } -> std::same_as<void>;
                { t.getHealth() } -> std::convertible_to<double>;
                { t.getPosition() } -> std::convertible_to<Vector3D>;
            };
        
        template<ComplexConcept T>
        class ComplexProcessor {
        public:
            void process(const T& entity) {
                // All these operations are guaranteed to be valid
                [[maybe_unused]] auto name = entity.getName();
                [[maybe_unused]] auto id = entity.getId();
                [[maybe_unused]] auto health = entity.getHealth();
                [[maybe_unused]] auto position = entity.getPosition();
            }
        };
        
        static_assert(ComplexConcept<Planet>);
        static_assert(ComplexConcept<Fleet>);
        static_assert(!ComplexConcept<int>);
        
        ComplexProcessor<Planet> planetProcessor;
        ComplexProcessor<Fleet> fleetProcessor;
        
        Planet testPlanet("ComplexPlanet", Vector3D{0, 0, 0});
        Fleet testFleet("ComplexFleet", Vector3D{0, 0, 0});
        
        REQUIRE_NOTHROW(planetProcessor.process(testPlanet));
        REQUIRE_NOTHROW(fleetProcessor.process(testFleet));
    }
}