#include "sqlite3.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>

struct searchResult {
    std::string bookId;
    int pos;
    std::string periText;
};

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    // Callback called if errors occur in the sqlite lib
    int i;

    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    };
    printf("\n");

    return 0;
}

int executePreparedStatement(sqlite3 *db, std::string sql, std::string *arguments)
{
    // Function used to simplify making prepared statements
    // @param: db - the database
    // @param: sql - SQL statement with placeholders
    // @param: arguments - list of arguments to replace placeholders with
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        sql.length(),
        &stmt,
        nullptr);

    for (int i = 0; i < sizeof(arguments); i++)
    {
        sqlite3_bind_text(
            stmt,
            i + 1,
            arguments[i].c_str(),
            arguments[i].length(),
            SQLITE_STATIC);
    };

    int rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (SQLITE_DONE != rc)
    {
        return 1;
    }
    else
    {
        return 0;
    };

    return rc;
}

std::pair<std::vector<std::vector<std::string>>, int> getResultsFromPreparedStatement(sqlite3 *db, std::string sql, std::string *arguments)
{
    // Function used to simplify making prepared statements and reading results
    // @param: db - the database
    // @param: sql - SQL statement with placeholders
    // @param: arguments - list of arguments to replace placeholders with
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        sql.length(),
        &stmt,
        nullptr);

    for (int i = 0; i < sizeof(arguments); i++)
    {
        sqlite3_bind_text(
            stmt,
            i + 1,
            arguments[i].c_str(),
            arguments[i].length(),
            SQLITE_STATIC);
    };

    // vector storing the rows returned
    std::vector<std::vector<std::string>> results;

    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
        int i;
        int num_cols = sqlite3_column_count(stmt);
        std::vector<std::string> curCol;

        for (i = 0; i < num_cols; i++)
        {

            switch (sqlite3_column_type(stmt, i))
            {
            case (SQLITE3_TEXT):
                curCol.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i))));
                break;
            default:
                break;
            };
        }

        results.push_back(curCol);
    }

    sqlite3_finalize(stmt);

    std::pair<std::vector<std::vector<std::string>>, int> res = {results, 0};

    return res;
}

int createTable(sqlite3 *db)
{
    // Function to create the table
    // @param: db - the database
    char *zErrMsg = 0;

    std::string arguments[0] = {};
    std::string sql = "CREATE TABLE fulltext(ID INTEGER PRIMARY KEY AUTOINCREMENT, bookId TEXT NOT NULL, bookName TEXT NOT NULL, text TEXT NOT NULL);";

    int rc = executePreparedStatement(db, sql, arguments);

    return rc;
}

std::pair<std::vector<std::vector<std::string>>, int> getBook(sqlite3 *db, std::string bookId)
{
    // Function to search for a book in the database
    // @param: db - the database
    // @param: bookId - the id of the book to edit
    char *zErrMsg = 0;

    std::string arguments[1] = {bookId};
    std::string sql = "SELECT * FROM fulltext WHERE bookId = ?;";

    auto res = getResultsFromPreparedStatement(db, sql, arguments);

    return res;
};

std::pair<std::vector<std::vector<std::string>>, int> getAllBooks(sqlite3 *db)
{
    // Function to get all books in the database
    // @param: db - the database
    char *zErrMsg = 0;

    std::string arguments[0] = {};
    std::string sql = "SELECT * FROM fulltext;";

    auto res = getResultsFromPreparedStatement(db, sql, arguments);

    return res;
};

int addBook(sqlite3 *db, std::string bookId, std::string bookName, std::string text)
{
    // Function to add a book
    // @param: db - the database
    // @param: bookId - the id of the book to edit
    // @param: bookName - the name of the book to add
    // @param: text - the text of the book to add
    char *zErrMsg = 0;

    std::string sql = "INSERT INTO fulltext(bookId, bookName, text) VALUES (?1, ?2, ?3);";
    std::string arguments[3] = {bookId, bookName, text};
    int rc = executePreparedStatement(db, sql, arguments);

    return rc;
};

int editBook(sqlite3 *db, std::string bookId, std::string bookName, std::string text)
{
    // Function to edit a book
    // @param: db - the database
    // @param: bookId - the id of the book to edit
    // @param: bookName - the name of the book to edit
    // @param: text - the text of the book to edit

    char *zErrMsg = 0;

    std::string sql = "UPDATE fulltext SET bookName = ?1, text = ?2 WHERE bookId = ?3;";
    std::string arguments[3] = {bookName, text, bookId};
    int rc = executePreparedStatement(db, sql, arguments);

    return rc;
};

int removeBook(sqlite3 *db, std::string bookId)
{
    // Function to remove a book
    // @param: db - the database
    // @param: bookId - the id of the book to remove
    char *zErrMsg = 0;

    std::string sql = "DELETE FROM fulltext WHERE bookId = ?1;";
    std::string arguments[1] = {bookId};
    int rc = executePreparedStatement(db, sql, arguments);

    return rc;
};

// for string delimiter
std::vector<std::string> split(std::string s, std::string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

std::string normaliseWord(std::string word)
{
    std::string result;

    for (auto c : word)
    {
        if (!std::ispunct(c))
            result += tolower(c);
    }

    return result;
}

bool checkWords(std::vector<std::string> words, std::vector<std::string> splitSearchText)
{
    int correctWords = 0;

    for (int i = 0; i < words.size(); i++)
    {
        auto normalisedWord = normaliseWord(words[i]);
        auto normalisedSearch = normaliseWord(splitSearchText[i]);

        if (normalisedWord == normalisedSearch)
            correctWords += 1;
    }

    if (correctWords == words.size())
    {
        return true;
    }

    return false;
}

std::pair<std::vector<searchResult>, int> searchBook(sqlite3 *db, std::string bookId, std::string searchText, int stopAfterOne)
{
    // Function to search for text in a single book
    // @param: db - the database
    // @param: bookId - the id of the book
    // @param: searchText - the text to search for
    // @param: stopAfterOne - argument specifying if function should continue to search after it found first result
    auto res = getBook(db, bookId);

    std::pair<std::vector<searchResult>, int> searchResults;

    if (res.second == 1) {
        searchResults.second = 1;
        return searchResults;
    }        
    if (res.first.size() == 0) {
        searchResults.second = 0;
        return searchResults;
    }

    std::string text = res.first[0][2];

    auto splitText = split(text, " ");
    auto splitTextLength = splitText.size();
    auto splitSearchText = split(searchText, " ");
    auto searchTextLength = splitSearchText.size();

    bool stop = false;

    for (int i = 0; i < splitText.size(); i++)
    {
        // Create list of next words to come
        std::vector<std::string> wordList;
        for (int j = 0; j < searchTextLength; j++)
        {
            if (i + j >= splitTextLength)
            {
                stop = true;
                break;
            }

            wordList.push_back(splitText[i + j]);
        }

        if (stop)
            break;

        // Check if words match by using function to have easy expandability
        if (checkWords(wordList, splitSearchText))
        {
            std::string wordListStr = "";
            for (auto word : wordList)
            {
                wordListStr += word;
                wordListStr += " ";
            }
            searchResult sR{bookId, i, wordListStr};
            searchResults.first.push_back(sR);

            if (stopAfterOne)
            {
                searchResults.second = 0;
                return searchResults;
            }
        }
    }

    searchResults.second = 0;
    return searchResults;
};

std::pair<std::vector<searchResult>, int> searchAllBooks(sqlite3 *db, std::string searchText, bool stopAfterOne)
{
    // Function to search for text in all book
    // @param: db - the database
    // @param: searchText - the text to search for
    // @param: stopAfterOne - argument specifying if function should continue to search after it found first result
    auto res = getAllBooks(db);

    std::pair<std::vector<searchResult>, int> searchResults;

    if(res.second == 1) {
        searchResults.second = 1;
        return searchResults;
    }
    if(res.first.size() == 0) {
        searchResults.second = 0;
        return searchResults;
    }

    for (auto bookId : res.first)
    {
        auto res = searchBook(db, bookId[0], searchText, stopAfterOne);
        if(res.second == 1) {
            res.second = 1;
            return searchResults;
        }
        if (res.first.size() > 0)
        {
            std::copy(res.first.begin(), res.first.end(), std::back_inserter(searchResults.first));
        }
    }

    searchResults.second = 0;
    return searchResults;
};

sqlite3 *initDB()
{
    const char *dbName = "./db/fulltext.db";

    sqlite3 *db;
    // Open the database specified in command line arguments or open the default one
    int res = sqlite3_open(dbName, &db);

    if (res)
    {
        //database failed to open
        std::cout << "Database failed to open" << std::endl;
    }
    else
    {
        // Create Table
        createTable(db);
    };

    return db;
};

int deinitDB(sqlite3 *db)
{
    sqlite3_close(db);
    return 0;
};