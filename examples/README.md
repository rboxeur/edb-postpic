# Examples for edb-postpic

Included files:
- sample-image.png  (example PNG image, 800x600)
- postpic_example.sql  (example SQL script)

Quick instructions:
1. Place the example PNG in a directory readable by the PostgreSQL server process.
2. Adjust file paths inside postpic_example.sql to match your server environment.
3. Run the script with psql:
   psql -d your_database -f path/to/postpic_example.sql
4. Check exported files at the path specified in the script (for example, sample-image-export.png).

Notes:
- This example uses only PNG/JPEG formats as requested.
- Ensure the postpic extension is installed and available in the target database.
- If any helper functions (thumbnail, supported_formats, etc.) are missing in your build, the script handles that gracefully with exception blocks.

