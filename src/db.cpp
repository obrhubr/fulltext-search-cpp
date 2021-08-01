#include "sqlite3.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>

// struct to make passing around the results between functions easier
struct searchResult {
    std::string bookId;
    int pos;
    std::string periText;
};

struct searchResults {
    int errorCode;
    std::vector<searchResult> results;
};

// struct to make passing around rows returned from SQL queries easier
struct SQLRow {
    std::vector<std::string> row;
};

// struct to make passing results returned from SQL queries easier
struct SQLResults {
    int errorCode;
    std::vector<SQLRow> results;
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

    // Check for errors
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

SQLResults getResultsFromPreparedStatement(sqlite3 *db, std::string sql, std::string *arguments)
{
    // Function used to simplify making prepared statements and reading results
    // @param: db - the database
    // @param: sql - SQL statement with placeholders
    // @param: arguments - list of arguments to replace placeholders with
    sqlite3_stmt *stmt;

    // Prepare sql statement and bind the arguments to it
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
    std::vector<SQLRow> results;

    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
        int i;
        int num_cols = sqlite3_column_count(stmt);
        SQLRow curCol;

        for (i = 0; i < num_cols; i++)
        {

            switch (sqlite3_column_type(stmt, i))
            {
            case (SQLITE3_TEXT):
                // Push the returned text to the current row
                curCol.row.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i))));
                break;
            default:
                break;
            };
        }

        // Push the row into the results vector
        results.push_back(curCol);
    }

    sqlite3_finalize(stmt);

    SQLResults res;
    res.results = results;
    res.errorCode = 0;

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

SQLResults getBook(sqlite3 *db, std::string bookId)
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

SQLResults getAllBooks(sqlite3 *db)
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
    // Function to remove string delimiter copied from stackoverflow
    // @param: s - the string to split
    // @param: delimiter - the string to split at

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
    // Function to normalise text : remove punctuation, make lowercase
    // @param: word - the word to normalise

    std::string result;

    for (auto c : word)
    {
        if (!std::ispunct(c))
            result += tolower(c);
    }

    return result;
}

bool checkMatch(std::string normalisedWord, std::string normalisedSearch) {
    // Check if words are longer than 0
    if(normalisedWord.size() == 0 || normalisedSearch.size() == 0)
    {
        return false;
    }

    // Check normalised versions of each word against each other
    if (normalisedWord == normalisedSearch) return true;

    // Check if normalisedSearch is contained within normalisedWord
    if(normalisedWord.size() > 4)
    {
        if(abs(normalisedSearch.size() - normalisedWord.size()) < 3)
        {
            if (normalisedWord.find(normalisedSearch) != std::string::npos)
            {
                return true;
            };
        };

        if(abs(normalisedSearch.size() - normalisedWord.size()) < 3)
        {
            if (normalisedSearch.find(normalisedWord) != std::string::npos)
            {
                return true;
            };
        };
    };

    return false;
}

bool checkMutations(std::string normalisedWord, std::string normalisedSearch) {
    // Check if mutations of normalisedSearch match
    std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
    for(int i = 0; i < normalisedSearch.size(); i++) {
        // 27 not 26, to replace by nothing too
        for(int j = 0; j < 27; j++) {
            auto mutatedSearch = normalisedSearch;

            if (checkMatch(normalisedWord, mutatedSearch.replace(i, 1, alphabet.substr(j, 1)))) return true;
        }
    }
    return false;
}

bool checkWords(std::vector<std::string> splitTextWordList, std::vector<std::string> splitSearchText)
{
    int correctWords = 0;

    // Loop through all the words
    for (int i = 0; i < splitTextWordList.size(); i++)
    {
        // normalise both words to maximise chances of a match
        auto normalisedWord = normaliseWord(splitTextWordList[i]);
        auto normalisedSearch = normaliseWord(splitSearchText[i]);

        if(checkMatch(normalisedWord, normalisedSearch))
        {
            correctWords++;
            continue;
        }

        if(checkMutations(normalisedWord, normalisedSearch))
        {
            correctWords++;
            continue;
        }
    }

    // The number of matching words has to be the same as the number of words in the search query
    if (correctWords == splitTextWordList.size())
    {
        return true;
    }

    return false;
}

searchResults searchBook(sqlite3 *db, std::string bookId, std::string searchText, int stopAfterOne)
{
    // Function to search for text in a single book
    // @param: db - the database
    // @param: bookId - the id of the book
    // @param: searchText - the text to search for
    // @param: stopAfterOne - argument specifying if function should continue to search after it found first result
    SQLResults res = getBook(db, bookId);

    searchResults sRes;

    if (res.errorCode == 1) {
        sRes.errorCode = 1;
        return sRes;
    }        
    if (res.results.size() == 0) {
        sRes.errorCode = 0;
        return sRes;
    }

    std::string text = res.results[0].row[2];

    auto splitText = split(text, " ");
    auto splitTextLength = splitText.size();
    auto splitSearchText = split(searchText, " ");
    auto searchTextLength = splitSearchText.size();

    bool stop = false;

    for (int i = 0; i < splitText.size(); i++)
    {
        // Create list of next words to come
        std::vector<std::string> splitTextWordList;
        for (int j = 0; j < searchTextLength; j++)
        {
            if (i + j >= splitTextLength)
            {
                stop = true;
                break;
            }

            splitTextWordList.push_back(splitText[i + j]);
        }

        if (stop)
            break;

        // Check if words match by using function to have easy expandability
        if (checkWords(splitTextWordList, splitSearchText))
        {
            std::string splitTextWordListStr = "";
            for (auto word : splitTextWordList)
            {
                splitTextWordListStr += word;
                splitTextWordListStr += " ";
            }
            searchResult sR{bookId, i, splitTextWordListStr};
            sRes.results.push_back(sR);

            if (stopAfterOne)
            {
                sRes.errorCode = 0;
                return sRes;
            }
        }
    }

    sRes.errorCode = 0;
    return sRes;
};

searchResults searchAllBooks(sqlite3 *db, std::string searchText, bool stopAfterOne)
{
    // Function to search for text in all book
    // @param: db - the database
    // @param: searchText - the text to search for
    // @param: stopAfterOne - argument specifying if function should continue to search after it found first result
    auto res = getAllBooks(db);

    searchResults searchResults;

    if(res.errorCode == 1) {
        searchResults.errorCode = 1;
        return searchResults;
    }
    if(res.results.size() == 0) {
        searchResults.errorCode = 0;
        return searchResults;
    }

    for (auto bookId : res.results)
    {
        auto res = searchBook(db, bookId.row[0], searchText, stopAfterOne);
        if(res.errorCode == 1) {
            res.errorCode = 1;
            return searchResults;
        }
        if (res.results.size() > 0)
        {
            std::copy(res.results.begin(), res.results.end(), std::back_inserter(searchResults.results));
        }
    }

    searchResults.errorCode = 0;
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