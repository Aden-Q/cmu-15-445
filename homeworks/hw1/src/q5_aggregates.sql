SELECT CategoryName, COUNT(1) AS cnt, ROUND(AVG(UnitPrice), 2), MIN(UnitPrice), MAX(UnitPrice), CASE WHEN COUNT(1) > 10 THEN SUM(UnitsOnOrder) ELSE 0 END
FROM Category INNER JOIN Product ON Category.Id = Product.CategoryId
GROUP BY CategoryName;