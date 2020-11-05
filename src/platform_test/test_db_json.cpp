
/**
 * @file src/platform_test/test_db_json.cpp
 *
 * @brief Implements tests for @ref
 * leviathan::platform::db_json class
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project

// Headers from other projects
#include <platform/db_json.hpp>

// Headers from third party libraries
#include <Poco/TemporaryFile.h>

// Headers from standard libraries
#include <cstring>
#include <iostream>

// Forward declarations
namespace leviathan {
        namespace platform_test{
                void test_case_1();
                void test_case_2();
                void test_case_3();
                void test_db_json();
        }
}

void leviathan::platform_test::test_case_1()
{
        std::cout << "--- Starting test case 1 ---" << std::endl;
        namespace P = leviathan::platform;
        Poco::TemporaryFile tmp;
        P::db_json db(tmp.path(), 0, 5, 0);
        poco_assert(! db.look_up("any_uuid"));
        std::string content = "the_content_of_uuid";
        P::default_package p;
        p.resize((unsigned int)content.size());
        p.set_payload(content);
        db.create("uuid", p);
        poco_assert(db.look_up("uuid"));
        poco_assert(0 == std::strcmp(content.c_str(), db.get("uuid").message.get()));
        std::cout << "--- PASS ---" << std::endl;
}

void leviathan::platform_test::test_case_2()
{
        std::cout << "--- Starting test case 2 ---" << std::endl;
        namespace P = leviathan::platform;
        Poco::TemporaryFile tmp;
        P::db_json db(tmp.path(), 0, 5, 0);
        std::string content1 = "the_content_of_uuid1";
        std::string content2 = "the_content_of_uuid2";
        std::string content3 = "the_content_of_uuid3";
        P::default_package p1;
        P::default_package p2;
        P::default_package p3;

        p1.resize((unsigned int)content1.size());
        p1.set_payload(content1);

        p2.resize((unsigned int)content2.size());
        p2.set_payload(content2);

        p3.resize((unsigned int)content3.size());
        p3.set_payload(content3);

	db.create("uuid1", p1);
	db.create("uuid2", p2);
	db.create("uuid3", p3);

	poco_assert(db.look_up("uuid1"));
	poco_assert(db.look_up("uuid2"));
	poco_assert(db.look_up("uuid3"));

	poco_assert(0 == std::strcmp(content1.c_str(), db.get("uuid1").message));
	poco_assert(0 == std::strcmp(content2.c_str(), db.get("uuid2").message));
	poco_assert(0 == std::strcmp(content3.c_str(), db.get("uuid3").message));
        std::cout << "--- PASS ---" << std::endl;
}

void leviathan::platform_test::test_case_3()
{
        std::cout << "--- Starting test case 3 ---" << std::endl;
	namespace P = leviathan::platform;
        Poco::TemporaryFile tmp;
	P::db_json* db1 = new P::db_json(tmp.path(), 0, 5, 0);
	poco_assert(! db1->look_up("any_uuid"));
        std::string content = "the_content_of_uuid";
        P::default_package p;
        p.resize((unsigned int)content.size());
        p.set_payload(content);
	db1->create("uuid", p);
	poco_assert(db1->look_up("uuid"));
	poco_assert(0 == std::strcmp(content.c_str(), db1->get("uuid").message));
        delete db1;
	P::db_json* db2 = new P::db_json(tmp.path(), 0, 5, 0);
	poco_assert(db2->look_up("uuid"));
	poco_assert(0 == std::strcmp(content.c_str(), db2->get("uuid").message));
        delete db2;
        std::cout << "--- PASS ---" << std::endl;
}

void leviathan::platform_test::test_db_json()
{	
        std::cout << "--- Starting JSON DB test ---" << std::endl;
        leviathan::platform_test::test_case_1();
        leviathan::platform_test::test_case_2();
        leviathan::platform_test::test_case_3();
        std::cout << "--- Finish JSON DB test ---" << std::endl;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
