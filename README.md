# Mini In-Memory Database Engine

A lightweight Redis-inspired key-value database implemented in modern C++. The project supports persistent storage, Write-Ahead Logging (WAL), transactions, TTL-based key expiration, range scans, and thread-safe operations.

The primary goal of this project is to demonstrate how a simple database engine can be built from scratch while exploring important systems concepts such as persistence, crash recovery, transactions, synchronization, and file-based storage.

---

# Features

* Key-Value Storage
* TTL (Time-To-Live) Support
* Write-Ahead Logging (WAL)
* Snapshot Persistence
* Transactions (BEGIN / COMMIT / ROLLBACK)
* Range Queries
* Thread-Safe Operations
* Automatic Expired Key Cleanup
* Crash Recovery

---

# Architecture

The system is organized into multiple layers:

Parser -> Transaction Manager -> Write Ahead Log (WAL) -> Database Storage Engine

### Parser

Converts user commands into structured Command objects.

### Transaction Manager

Buffers operations until the user commits or rolls them back.

### WAL (Write-Ahead Log)

Stores all modifying operations before they are applied permanently.

### Database Engine

Maintains the actual key-value store and provides querying capabilities.

---

# Project Structure

## database.h / database.cpp

These files implement the core storage engine.

### Responsibilities

* Store key-value pairs
* Manage TTL information
* Handle key expiration
* Perform scans and range scans
* Save snapshots to disk
* Load snapshots from disk
* Apply committed transactions
* Ensure thread-safe access

### Important Functions

#### set()

Inserts or updates a key-value pair.

#### get()

Retrieves the value associated with a key.

#### del()

Deletes a key from the database.

#### exists()

Checks whether a key currently exists.

#### scan()

Returns all keys in sorted order.

#### rangeScan()

Returns keys within a specified range of strings.

#### cleanupExpired()

Removes expired keys which functions upon every 5 seconds.

#### save()

Writes the current database state to a snapshot file.

#### load()

Restores database state from a snapshot file.

#### applyOperationsToBatch()

Applies all operations collected inside a committed transaction.

---

## wal.h / wal.cpp

Implements Write-Ahead Logging.

The WAL acts as a recovery mechanism if the application terminates unexpectedly.

### Responsibilities

* Log all modifying operations
* Recover operations after crashes
* Maintain durability

### Important Functions

#### appendSet()

Logs a SET operation.

#### appendDel()

Logs a DEL operation.

#### replay()

Replays all operations stored in the WAL during startup.

#### clear()

Clears the WAL.log file after a successful snapshot save.

---

## transaction.h / transaction.cpp

Implements transaction support.

Transactions allow multiple operations to be grouped together and executed atomically.

### Responsibilities

* Buffer operations
* Commit operations
* Roll back operations
* Maintain transaction state

### Important Functions

#### set()

Adds a SET operation to the transaction buffer.

#### del()

Adds a DELETE operation to the transaction buffer.

#### commit()

Writes all buffered operations to the WAL and applies them to the database.

#### rollback()

Discards all buffered operations.

#### active()

Checks whether a transaction is currently active.

---

## parser.h / parser.cpp

Responsible for parsing user commands.

### Responsibilities

* Read user input
* Validate syntax
* Generate command objects

### Supported Commands

SET
GET
DEL
EXISTS
SCAN
RANGE
SAVE
LOAD
BEGIN
COMMIT
ROLLBACK
EXIT

---

## main.cpp

Application entry point.

### Responsibilities

* Initialize the database
* Recover data from snapshots and WAL
* Launch the TTL cleanup thread
* Process user commands
* Manage transaction lifecycle
* Handle application shutdown

---

# Persistence Model

The database uses **TWO persistence mechanisms**.

## Snapshot

A snapshot contains the complete state of the database at the moment it is created.

Example:

snapshot.db

name "Karunya" -1

age "20" -1

Snapshots are generated through:

SAVE snapshot.db

or automatically when exiting the application.

---

## Write-Ahead Log (WAL)

The Write-Ahead Log (WAL) records every database modification("SET" and "DEL") that occurs after the latest snapshot, enabling recovery in the event of an unexpected shutdown.
(The WAL helps recover committed changes after a crash. Right now it is file-based, but a real database would use stronger syncing to make sure the data is definitely written to disk.)
Example:

SAVE snapshot.db

BEGIN

SET newkey 25

GET newkey

NULL

COMMIT

(suppose process is killed at this moment)

wal.log
SET newkey "25" -1

(ignore -1 in the log file for now. Its purpose will be explained later.)
The WAL enables recovery if the program crashes before a new snapshot is generated.

---

# Transaction Workflow

Example:

BEGIN

SET name Karunya

SET age 20

COMMIT

Before COMMIT:

* Changes exist only inside the transaction buffer.
* The database remains unchanged.

After COMMIT:

* Operations are written to the WAL.
* Operations are applied to the database.
* The transaction is closed.

Example rollback:

BEGIN

SET city Chennai

ROLLBACK

The database remains unchanged.

---

# TTL Support

Keys can be assigned an expiration time.

Example:

SET age 20 TTL 30

The key will automatically expire after 30 seconds.

Expired keys are removed through:

* Background cleanup thread.
* Lazy expiration during GET and EXISTS operations.

---

# Crash Recovery

Startup procedure:

1. Load the most recent snapshot.
2. Replay all operations stored in the WAL.

This guarantees that committed changes survive unexpected shutdowns.

Recovery sequence:

load(snapshot.db) -> replay(wal.log) -> database restored

---

# Build Instructions

Using g++:

g++ -std=c++17 main.cpp database.cpp parser.cpp wal.cpp transaction.cpp -o minidb.exe

Run:

./minidb.exe

---

# Example Usage

SET name Karunya

GET name

Karunya

SET age 20 TTL 10

GET age

20

BEGIN

SET name Adwaith

COMMIT

GET name

Adwaith

GET age (Assume this operation is done atleast 30 seconds after the SET age operation)

NULL

---

# Concepts Demonstrated

* Object-Oriented Programming
* Smart Pointers (unique_ptr)
* STL Containers
* Multithreading
* Mutex Synchronization
* Atomic Variables
* File I/O
* Serialization
* Transactions
* Write-Ahead Logging
* Snapshot Persistence
* Crash Recovery
* TTL-Based Storage

---

# Future Improvements

* Multiple Concurrent Clients
* Reader-Writer Locks
* B+ Tree Indexing
* Network Interface

---

This project was developed as an educational exploration of database internals and storage engine design using modern C++.
