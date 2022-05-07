WITH cte AS
(
    SELECT IFNULL(CompanyName, 'MISSING_NAME') AS CompanyName, 'Order'.Id AS OrderId, Customer.Id As CustomerId
    FROM 'Order' LEFT OUTER JOIN Customer ON 'Order'.CustomerId = Customer.Id
)
SELECT CompanyName, CustomerId, total_expenditure
FROM
(
    SELECT CompanyName, CustomerId, ROUND(SUM(UnitPrice * Quantity), 2) AS total_expenditure, NTILE(4) OVER(ORDER BY SUM(UnitPrice * Quantity)) bucket_number
    FROM cte, OrderDetail
    WHERE cte.OrderId = OrderDetail.OrderId
    GROUP BY CustomerId
)
WHERE bucket_number >= 2;