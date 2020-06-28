#include <alia/flow/object_trees.hpp>

#include <alia/flow/events.hpp>

#include <flow/testing.hpp>

namespace {

struct test_object
{
    void
    remove()
    {
        the_log << "removing " << name << "; ";

        auto& siblings = this->parent->children;
        siblings.erase(
            std::remove(siblings.begin(), siblings.end(), this),
            siblings.end());
    }

    void
    relocate(test_object& new_parent, test_object* after, test_object* before)
    {
        the_log << "relocating " << name << " into " << new_parent.name;
        if (after)
            the_log << " after " << after->name;
        the_log << "; ";

        if (this->parent)
        {
            auto& siblings = this->parent->children;
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), this),
                siblings.end());
        }

        this->parent = &new_parent;

        auto& siblings = new_parent.children;
        std::vector<test_object*>::iterator insertion_point;
        if (after)
        {
            insertion_point = siblings.insert(
                std::find(siblings.begin(), siblings.end(), after) + 1, this);
        }
        else
        {
            insertion_point = siblings.insert(siblings.begin(), this);
        }

        ++insertion_point;
        if (before)
        {
            REQUIRE(*insertion_point == before);
        }
        else
        {
            REQUIRE(insertion_point == siblings.end());
        }
    }

    void
    stream(std::ostream& out)
    {
        out << name << "(";
        for (test_object* child : children)
        {
            child->stream(out);
            out << ";";
        }
        out << ")";
    }

    std::string
    to_string()
    {
        std::stringstream out;
        this->stream(out);
        return out.str();
    }

    std::string name;
    test_object* parent = nullptr;
    std::vector<test_object*> children;
};

ALIA_DEFINE_TAGGED_TYPE(tree_traversal_tag, tree_traversal<test_object>&)

typedef alia::extend_context_type_t<alia::context, tree_traversal_tag>
    test_context;

void
do_object(test_context ctx, std::string name)
{
    tree_node<test_object>* node;
    if (get_cached_data(ctx, &node))
        node->object.name = name;
    if (is_refresh_event(ctx))
        refresh_tree_node(get<tree_traversal_tag>(ctx), *node);
}

template<class Contents>
void
do_container(test_context ctx, std::string name, Contents contents)
{
    scoped_tree_node<test_object> scoped;
    tree_node<test_object>* node;
    if (get_cached_data(ctx, &node))
        node->object.name = name;
    if (is_refresh_event(ctx))
        scoped.begin(get<tree_traversal_tag>(ctx), *node);
    contents(ctx);
}

template<class Contents>
void
do_piecewise_container(test_context ctx, std::string name, Contents contents)
{
    tree_node<test_object>* node;
    if (get_cached_data(ctx, &node))
        node->object.name = name;
    if (is_refresh_event(ctx))
        refresh_tree_node(get<tree_traversal_tag>(ctx), *node);
    scoped_tree_children<test_object> scoped;
    if (is_refresh_event(ctx))
        scoped.begin(get<tree_traversal_tag>(ctx), *node);
    contents(ctx);
}

} // namespace

TEST_CASE("simple object tree", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_object(ctx, "bit0");
        }
        ALIA_END

        ALIA_IF(n & 2)
        {
            do_object(ctx, "bit1");
        }
        ALIA_END

        ALIA_IF(n & 4)
        {
            do_object(ctx, "bit2");
        }
        ALIA_END

        ALIA_IF(n & 8)
        {
            do_object(ctx, "bit3");
        }
        ALIA_END

        ALIA_IF(n & 16)
        {
            do_object(ctx, "bit4");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = vanilla_ctx.add<tree_traversal_tag>(traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    n = 0;
    refresh_system(sys);
    check_log("");
    REQUIRE(root.object.to_string() == "root()");

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into root after bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();)");

    n = 0;
    refresh_system(sys);
    check_log(
        "removing bit0; "
        "removing bit1; ");
    REQUIRE(root.object.to_string() == "root()");

    n = 2;
    refresh_system(sys);
    check_log("relocating bit1 into root; ");
    REQUIRE(root.object.to_string() == "root(bit1();)");

    n = 15;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit2 into root after bit1; "
        "relocating bit3 into root after bit2; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();bit2();bit3();)");

    n = 13;
    refresh_system(sys);
    check_log("removing bit1; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit2();bit3();)");

    n = 2;
    refresh_system(sys);
    check_log(
        "removing bit0; "
        "relocating bit1 into root; "
        "removing bit2; "
        "removing bit3; ");
    REQUIRE(root.object.to_string() == "root(bit1();)");
}

TEST_CASE("multilevel object tree", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_container(ctx, "bit0", [&](test_context ctx) {
                ALIA_IF(n & 2)
                {
                    do_object(ctx, "bit1");
                }
                ALIA_END

                ALIA_IF(n & 4)
                {
                    do_container(ctx, "bit2", [&](test_context ctx) {
                        ALIA_IF(n & 8)
                        {
                            do_object(ctx, "bit3");
                        }
                        ALIA_END

                        ALIA_IF(n & 16)
                        {
                            do_object(ctx, "bit4");
                        }
                        ALIA_END
                    });
                }
                ALIA_END

                ALIA_IF(n & 32)
                {
                    do_object(ctx, "bit5");
                }
                ALIA_END
            });
        }
        ALIA_END

        ALIA_IF(n & 64)
        {
            do_object(ctx, "bit6");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = vanilla_ctx.add<tree_traversal_tag>(traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0(bit1(););)");

    n = 64;
    refresh_system(sys);
    check_log(
        "removing bit1; "
        "removing bit0; "
        "relocating bit6 into root; ");
    REQUIRE(root.object.to_string() == "root(bit6();)");

    n = 125;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit2 into bit0; "
        "relocating bit3 into bit2; "
        "relocating bit4 into bit2 after bit3; "
        "relocating bit5 into bit0 after bit2; ");
    REQUIRE(
        root.object.to_string()
        == "root(bit0(bit2(bit3();bit4(););bit5(););bit6();)");

    n = 55;
    refresh_system(sys);
    check_log(
        "relocating bit1 into bit0; "
        "removing bit3; "
        "removing bit6; ");
    REQUIRE(
        root.object.to_string() == "root(bit0(bit1();bit2(bit4(););bit5(););)");
}

TEST_CASE("fluid object tree", "[flow][object_trees]")
{
    clear_log();

    tree_node<test_object> root;
    root.object.name = "root";

    std::vector<std::string> a_team, b_team;

    auto controller = [&](test_context ctx) {
        naming_context nc(ctx);
        do_container(ctx, "a_team", [&](test_context ctx) {
            for (auto& name : a_team)
            {
                named_block nb(nc, make_id(name));
                do_object(ctx, name);
            }
        });
        do_container(ctx, "b_team", [&](test_context ctx) {
            for (auto& name : b_team)
            {
                named_block nb(nc, make_id(name));
                do_object(ctx, name);
            }
        });
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = vanilla_ctx.add<tree_traversal_tag>(traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    a_team = {"alf", "betty", "charlie", "dot"};
    b_team = {"edgar"};
    refresh_system(sys);
    check_log(
        "relocating a_team into root; "
        "relocating alf into a_team; "
        "relocating betty into a_team after alf; "
        "relocating charlie into a_team after betty; "
        "relocating dot into a_team after charlie; "
        "relocating b_team into root after a_team; "
        "relocating edgar into b_team; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(alf();betty();charlie();dot(););b_team(edgar(););)");

    a_team = {"betty", "charlie", "dot"};
    b_team = {"alf", "edgar"};
    refresh_system(sys);
    check_log(
        "relocating betty into a_team; "
        "relocating charlie into a_team after betty; "
        "relocating dot into a_team after charlie; "
        "removing alf; "
        "relocating alf into b_team; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(betty();charlie();dot(););b_team(alf();edgar(););)");

    a_team = {"betty", "charlie"};
    b_team = {"alf", "edgar"};
    refresh_system(sys);
    check_log("removing dot; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(betty();charlie(););b_team(alf();edgar(););)");

    a_team = {"betty", "edgar", "charlie"};
    b_team = {"alf"};
    refresh_system(sys);
    check_log("relocating edgar into a_team after betty; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(betty();edgar();charlie(););b_team(alf(););)");

    a_team = {"charlie", "dot", "betty", "edgar"};
    b_team = {"alf"};
    refresh_system(sys);
    check_log(
        "relocating charlie into a_team; "
        "relocating dot into a_team after charlie; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(charlie();dot();betty();edgar(););b_team(alf(););)");

    a_team = {"edgar", "dot", "charlie", "alf"};
    b_team = {"betty"};
    refresh_system(sys);
    check_log(
        "relocating edgar into a_team; "
        "relocating dot into a_team after edgar; "
        "relocating alf into a_team after charlie; "
        "removing betty; "
        "relocating betty into b_team; ");
    REQUIRE(
        root.object.to_string()
        == "root(a_team(edgar();dot();charlie();alf(););b_team(betty(););)");
}

TEST_CASE("piecewise containers", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_piecewise_container(ctx, "bit0", [&](test_context ctx) {
                ALIA_IF(n & 2)
                {
                    do_object(ctx, "bit1");
                }
                ALIA_END

                ALIA_IF(n & 4)
                {
                    do_piecewise_container(ctx, "bit2", [&](test_context ctx) {
                        ALIA_IF(n & 8)
                        {
                            do_object(ctx, "bit3");
                        }
                        ALIA_END

                        ALIA_IF(n & 16)
                        {
                            do_object(ctx, "bit4");
                        }
                        ALIA_END
                    });
                }
                ALIA_END

                ALIA_IF(n & 32)
                {
                    do_object(ctx, "bit5");
                }
                ALIA_END
            });
        }
        ALIA_END

        ALIA_IF(n & 64)
        {
            do_object(ctx, "bit6");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = vanilla_ctx.add<tree_traversal_tag>(traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into bit0; ");
    REQUIRE(root.object.to_string() == "root(bit0(bit1(););)");

    n = 64;
    refresh_system(sys);
    check_log(
        "removing bit1; "
        "removing bit0; "
        "relocating bit6 into root; ");
    REQUIRE(root.object.to_string() == "root(bit6();)");

    n = 125;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit2 into bit0; "
        "relocating bit3 into bit2; "
        "relocating bit4 into bit2 after bit3; "
        "relocating bit5 into bit0 after bit2; ");
    REQUIRE(
        root.object.to_string()
        == "root(bit0(bit2(bit3();bit4(););bit5(););bit6();)");

    n = 55;
    refresh_system(sys);
    check_log(
        "relocating bit1 into bit0; "
        "removing bit3; "
        "removing bit6; ");
    REQUIRE(
        root.object.to_string() == "root(bit0(bit1();bit2(bit4(););bit5(););)");
}

TEST_CASE("object tree caching", "[flow][object_trees]")
{
    clear_log();

    int n = 0;

    tree_node<test_object> root;
    root.object.name = "root";

    auto controller = [&](test_context ctx) {
        ALIA_IF(n & 1)
        {
            do_object(ctx, "bit0");
        }
        ALIA_END

        ALIA_IF(n & 2)
        {
            do_object(ctx, "bit1");
        }
        ALIA_END

        auto& caching_data
            = get_cached_data<tree_caching_data<test_object>>(ctx);
        scoped_tree_cacher<test_object> cacher(
            get<tree_traversal_tag>(ctx), caching_data, make_id(n & 12), false);
        ALIA_EVENT_DEPENDENT_IF(cacher.content_traversal_required())
        {
            the_log << "traversing cached content; ";

            ALIA_IF(n & 4)
            {
                do_object(ctx, "bit2");
            }
            ALIA_END

            ALIA_IF(n & 8)
            {
                do_object(ctx, "bit3");
            }
            ALIA_END
        }
        ALIA_END

        ALIA_IF(n & 16)
        {
            do_object(ctx, "bit4");
        }
        ALIA_END
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = vanilla_ctx.add<tree_traversal_tag>(traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    n = 0;
    refresh_system(sys);
    check_log("traversing cached content; ");
    REQUIRE(root.object.to_string() == "root()");

    n = 3;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "relocating bit1 into root after bit0; "
        // This happens because the caching system can't currently handle moving
        // the cached content.
        "traversing cached content; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();)");

    n = 2;
    refresh_system(sys);
    check_log("removing bit0; ");
    REQUIRE(root.object.to_string() == "root(bit1();)");

    n = 15;
    refresh_system(sys);
    check_log(
        "relocating bit0 into root; "
        "traversing cached content; "
        "relocating bit2 into root after bit1; "
        "relocating bit3 into root after bit2; ");
    REQUIRE(root.object.to_string() == "root(bit0();bit1();bit2();bit3();)");

    n = 14;
    refresh_system(sys);
    check_log("removing bit0; ");
    REQUIRE(root.object.to_string() == "root(bit1();bit2();bit3();)");

    n = 6;
    refresh_system(sys);
    check_log(
        "traversing cached content; "
        "removing bit3; ");
    REQUIRE(root.object.to_string() == "root(bit1();bit2();)");
}