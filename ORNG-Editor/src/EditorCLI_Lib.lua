entity_array = {}

function evaluate(expression)
    local chunk, errorMessage = load("return " .. expression)
    
    if not chunk then
        error("Error in expression: " .. errorMessage)
    end
    
    return chunk()  -- Execute the compiled code and return the result
end

