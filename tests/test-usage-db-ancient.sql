create table usage (application text, entry text, timestamp datetime);
insert into usage (application, entry, timestamp) values ("testapp.desktop", "Shorties", datetime('now', 'utc', '-31 day'));
insert into usage (application, entry, timestamp) values ("testapp.desktop", "Shorties", datetime('now', 'utc', '-32 day'));
insert into usage (application, entry, timestamp) values ("testapp.desktop", "Shorties", datetime('now', 'utc', '-33 day'));
insert into usage (application, entry, timestamp) values ("testapp.desktop", "Longer Term", datetime('now', 'utc', '-1 year', '-31 day'));
insert into usage (application, entry, timestamp) values ("testapp.desktop", "Longer Term", datetime('now', 'utc', '-1 year', '-32 day'));
insert into usage (application, entry, timestamp) values ("testapp.desktop", "Longer Term", datetime('now', 'utc', '-1 year', '-33 day'));
