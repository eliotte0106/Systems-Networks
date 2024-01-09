from typing import List


class Solution:
    def canJump(self, nums):
        last_ind = len(nums) - 1
        last_second_ind = len(nums)-2

        for i in range(last_second_ind, -1, -1):
            if i + nums[i] >= last_ind:
                last_ind = i
        return last_ind == i
    
a = Solution()
print(a.canJump([2,3,1,2,6]))