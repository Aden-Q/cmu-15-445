SELECT DISTINCT ShipName, substr(ShipName, 0, instr(ShipName, '-'))
FROM 'Order'
WHERE ShipName LIKE '%-%'
ORDER BY ShipName;