SELECT Id, ShipCountry
FROM 'Order'
WHERE ShipCountry IN ('USA', 'Mexico', 'Canada')
ORDER BY Id ASC LIMIT 20 OFFSET 50;