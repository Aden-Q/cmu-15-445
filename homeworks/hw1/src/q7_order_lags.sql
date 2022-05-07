WITH cte AS
(
    SELECT Id, OrderDate, LAG(OrderDate, 1, 0) OVER(ORDER BY OrderDate) PreviousDate
    FROM 'Order'
    WHERE CustomerId = 'BLONP'
    LIMIT 10
)
SELECT Id, OrderDate, PreviousDate, ROUND(julianday(OrderDate) - julianday(PreviousDate), 2) AS diff
FROM cte;