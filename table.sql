CREATE TABLE temp (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS document (		
    id TEXT PRIMARY KEY,		
    name TEXT,		
    content TEXT,		
    embeddings BLOB		
);		
PRAGMA journal_mode = WAL;		

--system_cache
CREATE TABLE system_cache (
    id TEXT PRIMARY KEY,
    sql TEXT NOT NULL
);
