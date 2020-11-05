1. First you need to save "download_from_metax.html" file to metax server and
get it's uuid (e.g. uuid_1 = "ab4cc821-3269-4bf0-9209-7c026b1c2431")
2. Put above mentioned uuid_1 into "upload_into_metax.html" in variable named
"down_page_uuid"
3. Then save "upload_into_metax.html" file in metax server and get it's uuid_2
6. Open in web browser "https://metax.leviathan.am:7071/db/get?id=" + uuid_2
