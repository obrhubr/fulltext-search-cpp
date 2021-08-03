#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <memory>
#include <cstdlib>
#include "db.cpp"
#include <restbed>
#include <nlohmann/json.hpp>

using namespace restbed;
using json = nlohmann::json;

// make db a global variable to access it inside route handlers
sqlite3 *db;

std::string getJsonBody(const Bytes &body) {
    // Function to extract string from body, if we pass body.data() to the json parser directly it throws an error
    std::string jsonBody = "";
    for(auto c : body) {
        jsonBody += c;
    }
    return jsonBody;
}

void add_handler(const std::shared_ptr<Session> session)
{
    const auto request = session->get_request();

    auto length = 0;
    request->get_header("Content-Length", length);

    session->fetch(length, [](const std::shared_ptr<Session> session, const Bytes &body)
    {
        auto req = json::parse(getJsonBody(body));
        std::string res = " ";

        if(req["bookId"].is_string() && req["bookName"].is_string() && req["text"].is_string()) {
            std::cout << "Add book in sqlite. " << std::endl;
            int rc = addBook(db, req["bookId"], req["bookName"], req["text"]);
            if (rc == 0)
            {
                res = "{\"response\": \"Saved book to the database. \"}";
            } else {
                res = "{\"response\": \"Error while saving book to the database. \"}";
                session->close(500, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
                return;
            }
        } else {
            res = "{\"response\": \"Error while validating input. \"}";
            session->close(400, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
            return;
        }
        session->close(OK, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
    });
};

void edit_handler(const std::shared_ptr<Session> session)
{
    const auto request = session->get_request();

    auto length = 0;
    request->get_header("Content-Length", length);

    session->fetch(length, [](const std::shared_ptr<Session> session, const Bytes &body)
    {
        auto req = json::parse(getJsonBody(body));
        std::string res = " ";

        if(req["bookId"].is_string()) {
            if(req["bookName"].is_null() || req["text"].is_null()) {
                auto bookData = getBook(db, req["bookId"]);
                if(bookData.errorCode == 0) {
                    if(req["bookName"].is_null()) {
                        req["bookName"] = bookData.results[0].row[1];
                    };
                    if(req["text"].is_null()) {
                        req["text"] = bookData.results[0].row[2];
                    };
                } else {
                    res = "{\"response\": \"Error while querying database. \"}";
                    session->close(500, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
                    return;
                }
            }
            std::cout << "Editing book in sqlite. " << std::endl;
            int rc = editBook(db, req["bookId"], req["bookName"], req["text"]);
            
            if (rc == 0)
            {
                res = "{\"response\": \"Edited book and saved to the database. \"}";
            } else {
                res = "{\"response\": \"Error while editing book and saving to the database. \"}";
                session->close(500, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
                return;
            }
        } else {
            res = "{\"response\": \"Error while validating input. \"}";
                session->close(400, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
                return;
        }

        session->close(OK, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
    });
};

void remove_handler(const std::shared_ptr<Session> session)
{
    const auto request = session->get_request();

    auto length = 0;
    request->get_header("Content-Length", length);

    session->fetch(length, [](const std::shared_ptr<Session> session, const Bytes &body)
    {
        auto req = json::parse(getJsonBody(body));
        std::string res = " ";

        if(req["bookId"].is_string()) {
            std::cout << "Removing book from sqlite. " << std::endl;
            int rc = removeBook(db, req["bookId"]);

            if (rc == 0)
            {
                res = "{\"response\": \"Removed book from the database. \"}";
            } else {
                res = "{\"response\": \"Error while removing book from the database. \"}";
                session->close(500, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
                return;
            }
        } else {
            res = "{\"response\": \"Error while validating input. \"}";
            session->close(400, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
            return;
        }

        session->close(OK, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
    });
};

void removeAll_handler(const std::shared_ptr<Session> session)
{
    const auto request = session->get_request();

    auto length = 0;
    request->get_header("Content-Length", length);

    session->fetch(length, [](const std::shared_ptr<Session> session, const Bytes &body)
    {
        std::cout << "Removing all books from sqlite. " << std::endl;
        int rc = removeAllBooks(db);

        if (rc == 0)
        {
            res = "{\"response\": \"Removed all books from the database. \"}";
        } else {
            res = "{\"response\": \"Error while removing all books from the database. \"}";
            session->close(500, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
            return;
        }

        session->close(OK, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
    });
};

void search_one_handler(const std::shared_ptr<Session> session)
{
    const auto request = session->get_request();

    auto length = 0;
    request->get_header("Content-Length", length);

    session->fetch((size_t)length, [](const std::shared_ptr<Session> session, const Bytes &body)
    {
        auto req = json::parse(getJsonBody(body));
        std::string res = " ";

        if(req["bookId"].is_string() && req["searchText"].is_string() && req["stopAfterOne"].is_boolean()) {
            std::cout << "Searching book in sqlite, with term: " << req["searchText"] << std::endl;
            auto rc = searchBook(db, req["bookId"], req["searchText"], req["stopAfterOne"]);

            if (rc.errorCode == 0)
            {
                json searchRes;

                searchRes["results"] = {};

                for(auto sres : rc.results) {
                    json searchInfo;
                    searchInfo["bookId"] = sres.bookId;
                    searchInfo["word"] = sres.pos;
                    searchInfo["periText"] = sres.periText;
                    searchRes["results"].push_back(searchInfo);
                };

                res = searchRes.dump();
            } else {
                res = "{\"response\": \"Error while searching book in the database. \"}";
                session->close(500, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
                return;
            }
        } else {
            res = "{\"response\": \"Error while validating input. \"}";
            session->close(400, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
            return;
        }

        session->close(OK, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
    });
};

void search_all_handler(const std::shared_ptr<Session> session)
{
    const auto request = session->get_request();

    auto length = 0;
    request->get_header("Content-Length", length);

    session->fetch(length, [](const std::shared_ptr<Session> session, const Bytes &body)
    {
        auto req = json::parse(getJsonBody(body));
        std::string res = " ";

        if(req["searchText"].is_string() && req["stopAfterOne"].is_boolean()) {
            std::cout << "Searching all books in sqlite, with term: " << req["searchText"] << std::endl;
            auto rc = searchAllBooks(db, req["searchText"], req["stopAfterOne"]);
            
            if (rc.errorCode == 0)
            {
                json searchRes;

                searchRes["results"] = {};

                for(auto sres : rc.results) {
                    json searchInfo;
                    searchInfo["bookId"] = sres.bookId;
                    searchInfo["word"] = sres.pos;
                    searchInfo["periText"] = sres.periText;
                    searchRes["results"].push_back(searchInfo);
                };

                res = searchRes.dump();
            } else {
                res = "{\"response\": \"Error while searching books in the database. \"}";
                session->close(500, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
                return;
            }
        } else {
            std::cout << "Error while validating input." << std::endl;
            res = "{\"response\": \"Error while validating input. \"}";
            session->close(400, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
            return;
        }

        session->close(OK, res, {{"Content-Length", std::to_string(res.size())}, {"Content-type", "application/json"}});
    });
};


int main(const int, const char **)
{
    db = initDB();

    Service service;

    // Route to add text
    auto add_resource = std::make_shared<Resource>();
    add_resource->set_path("/add");
    add_resource->set_method_handler("POST", add_handler);
    service.publish(add_resource);

    // route to edit text
    auto edit_resource = std::make_shared<Resource>();
    edit_resource->set_path("/edit");
    edit_resource->set_method_handler("POST", edit_handler);
    service.publish(edit_resource);

    // route to remove text
    auto remove_resource = std::make_shared<Resource>();
    remove_resource->set_path("/remove");
    remove_resource->set_method_handler("POST", remove_handler);
    service.publish(remove_resource);

    // route to remove all texts
    auto removeAll_resource = std::make_shared<Resource>();
    removeAll_resource->set_path("/removeAll");
    removeAll_resource->set_method_handler("POST", removeAll_handler);
    service.publish(removeAll_resource);

    // route to search text in one book
    auto search_one_resource = std::make_shared<Resource>();
    search_one_resource->set_path("/search/one");
    search_one_resource->set_method_handler("POST", search_one_handler);
    service.publish(search_one_resource);
    
    // route to search text in all books
    auto search_all_resource = std::make_shared<Resource>();
    search_all_resource->set_path("/search/all");
    search_all_resource->set_method_handler("POST", search_all_handler);
    service.publish(search_all_resource);

    // Set up server
    auto settings = std::make_shared<Settings>();
    settings->set_port(1984);
    settings->set_default_header("Connection", "close");

    // Create and start server
    std::cout << "Starting server on port: " << settings->get_port() << std::endl;;
    service.start(settings);

    deinitDB(db);

    return EXIT_SUCCESS;
};