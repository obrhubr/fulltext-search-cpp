# Fulltext - search

### Description

#### Attention

This version is only slower than the master branch if the number of books is low.

This is a webservice, built in C++, offering a simple api to add, edit, remove and search books. I use it in my project [obrhubr/homelibrary](https://www.github.com/obrhubr/homelibrary).

### Table of Content

- [**Getting Started**](#getting-started)
- [Built With](#built-with)

### Getting Started

To get started, install `librestbed-dev`, `nlohmann-json3-dev` and `libsqlite3-dev` using your package manager. After completing installation, run `make`. This should build the executable.

### Usage

There are 5 routes. All of them accept only json: 
 - `/add` : Adding a book to make it searchable
 - `/edit` : Edit a book
 - `/remove` : Remove a book
 - `/search/one` : Search the text of a single book
 - `/search/all` : Seach the text of all books

#### `/add`
To use the `/add` route, send:
```
{
    "bookId": The Id of the book in your main database,
    "bookName": The name of the book,
    "text": The text in the book
}
```

#### `/edit`
To use the `/edit` route, send:
```
{
    "bookId": The Id of the book in your main database
    Only send the fields which you want to update(you can send nothing)
    "bookName": The name of the book,
    "text": The text in the book
}
```

#### `/remove`
To use the `/remove` route, send:
```
{
    "bookId": The Id of the book in your main database
}
```

#### `/search/one`
To use the `/search/one` route, send:
```
{
    "bookId": The Id of the book in your main database,
    "searchText": The text you want to search for,
    "stopAfterOne": (Boolean) If you want to stop after finding the first result
}
```
It will send back data resembling this:
```
{
    "results": [
        {
            "bookId": "1",
            "periText": "test ",
            "word": 0
        }
    ]
}
```

#### `/search/all`
To use the `/search/all` route, send:
```
{
    "searchText": The text you want to search for,
    "stopAfterOne": (Boolean) If you want to stop after finding the first result
}
```
It will send back data resembling this:
```
{
    "results": [
        {
            "bookId": "1",
            "periText": "test ",
            "word": 0
        }
    ]
}
```

#### Errors
If an error occurs, the response will look like this:
```
{
    "response": Error in text form
}
```


### Built With

This project was built using the official `sqlite3` adapter for C++, [nlohmann/json](https://www.github.com/nlohmann/json) to parse json and [corvusoft/restbed](https://github.com/Corvusoft/restbed) to create the webserver part of the project.