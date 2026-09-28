def inv(a, m) :  
    m0 = m  
    x0 = 0
    x1 = 1
    if (m == 1) :  
        return 0
    while (a > 1) :  
        q = a // m  
        t = m  
        m = a % m  
        a = t  
        t = x0  
        x0 = x1 - q * x0  
        x1 = t  
    if (x1 < 0) :  
        x1 = x1 + m0  
    return x1  
def findMinX(nums, rems, k) :  
    prod = 1
    for i in range(0, k) :  
        prod = prod * nums[i]  
    result = 0
    for i in range(0,k):  
        pp = prod // nums[i]  
        result = result + rems[i] * inv(pp, nums[i]) * pp  
    return result % prod