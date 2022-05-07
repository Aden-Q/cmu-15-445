SELECT Shipper.CompanyName, ROUND(SUM(CASE WHEN ShippedDate > RequiredDate THEN 1 ELSE 0 END) * 1.0 / COUNT(1), 2)
FROM 'Order', Shipper
WHERE ShipVia = Shipper.Id
GROUP BY Shipper.CompanyName;