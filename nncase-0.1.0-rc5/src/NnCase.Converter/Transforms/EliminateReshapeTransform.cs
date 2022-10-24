using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NnCase.Converter.Model;
using NnCase.Converter.Model.Layers;

namespace NnCase.Converter.Transforms
{
    public class EliminateReshapeTransform : Transform
    {
        protected override bool OnTryMatch(Layer layer, TransformContext context)
        {
            try
            {
                if (layer is Reshape reshape1 && reshape1.Input.Dimensions.SequenceEqual(reshape1.Output.Dimensions))
                {
                    context.MatchedLayers.Add(layer);
                    context.Inputs.Add(reshape1.Input);
                    context.Outputs.Add(reshape1.Output);

                    return true;
                }

                return false;
            }
            catch
            {
                return false;
            }
        }

        public override void Process(TransformContext context)
        {
            var reshape1 = (Reshape)context.MatchedLayers[0];
            var input = reshape1.Input.Connection.From;
            var output = reshape1.Output;

            var oldOuts = output.Connections.Select(o => o.To).ToList();
            foreach (var oldOut in oldOuts)
                oldOut.SetConnection(input);
        }
    }
}