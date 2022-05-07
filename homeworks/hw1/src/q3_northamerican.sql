WITH TempTable AS (
    SELECT ShipCountry
)

SELECT Id, ShipCountry
FROM 'Order', TempTable
WHERE ShipCountry IN ('USA', 'Mexico', 'Canada')
ORDER BY Id ASC LIMIT 20 OFFSET 50;