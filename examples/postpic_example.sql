-- examples/postpic_example.sql
-- Example usage of postpic functions (based on postpic--0.9.1.sql)
-- Uses only PNG/JPEG. Adjust file paths according to your PostgreSQL server.

-- 0. Version information
SELECT 'postpic_version' AS what, postpic_version() AS value;

-- 1. Create an example table
DROP TABLE IF EXISTS postpic_examples;
CREATE TABLE postpic_examples (
  id serial PRIMARY KEY,
  name text NOT NULL,
  the_img image  -- image type provided by the postpic extension
);

-- 2. Import a PNG image from the server filesystem
-- Replace the path with a real path readable by the postgres user
INSERT INTO postpic_examples (name, the_img)
VALUES (
  'sample-png',
  postpic_from_file('/var/lib/edb-as/examples/sample-image.png')
);

-- 3. Check format and dimensions
SELECT
  id,
  name,
  postpic_get_format(the_img) AS format,
  postpic_get_width(the_img) AS width,
  postpic_get_height(the_img) AS height
FROM postpic_examples;

-- 4. Create a thumbnail (example of a transformation function if available)
-- If a thumbnail function exists in your version, adapt the function name accordingly.
DO $$
DECLARE
  img image;
  thumb image;
BEGIN
  SELECT the_img INTO img FROM postpic_examples WHERE name = 'sample-png' LIMIT 1;
  IF img IS NULL THEN
    RAISE NOTICE 'sample-png image not found';
    RETURN;
  END IF;

  -- Generic example: create a 200x200 thumbnail if the function exists
  BEGIN
    thumb := postpic_thumbnail(img, 200, 200);
    INSERT INTO postpic_examples (name, the_img) VALUES ('sample-png-thumb', thumb);
    RAISE NOTICE 'Thumbnail created and inserted';
  EXCEPTION WHEN undefined_function THEN
    RAISE NOTICE 'postpic_thumbnail is not available in this version';
  END;
END;
$$ LANGUAGE plpgsql;

-- 5. Export an image from the database to a file (PNG)
DO $$
DECLARE
  img image;
BEGIN
  SELECT the_img INTO img FROM postpic_examples WHERE name = 'sample-png' LIMIT 1;
  IF img IS NULL THEN
    RAISE NOTICE 'No image to export';
    RETURN;
  END IF;

  BEGIN
    PERFORM postpic_to_file(img, '/var/lib/edb-as/examples/sample-image-export.png', 'png');
    RAISE NOTICE 'Attempted export to examples/sample-image-export.png';
  EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE 'Export failed: %', SQLERRM;
  END;
END;
$$ LANGUAGE plpgsql;

-- 6. Optional cleanup (uncomment if needed)
-- DROP TABLE IF EXISTS postpic_examples;

